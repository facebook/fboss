#!/usr/bin/env python3

import argparse
import http.server
import json
import socketserver
import urllib.parse
from typing import Any

from neteng.fboss.ctrl import FbossCtrl
from thrift.protocol.THeaderProtocol import THeaderProtocol
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport.TSocket import TSocket


def create_client(host: str, port: int = 5909, timeout: float = 15.0):
    socket = TSocket(host, port)
    socket.setTimeout(int(timeout * 1000))
    transport = THeaderTransport(socket)
    protocol = THeaderProtocol(transport)
    transport.open()
    client = FbossCtrl.Client(protocol)  # pyre-ignore[45]
    return client, transport


def _check_port_status(client) -> dict[str, Any]:
    port_statuses = client.getPortStatus([])
    port_infos = client.getAllPortInfo()
    port_results = {}
    up_count = 0
    down_count = 0
    for port_id, status in port_statuses.items():
        port_name = port_infos.get(port_id)
        name = port_name.name if port_name else str(port_id)
        port_results[name] = {
            "up": status.up,
            "enabled": status.enabled,
            "speed_mbps": status.speedMbps,
        }
        if status.up:
            up_count += 1
        else:
            down_count += 1
    return {
        "total_ports": len(port_results),
        "ports_up": up_count,
        "ports_down": down_count,
        "ports": port_results,
    }


def _check_switch_state(client) -> dict[str, Any]:
    run_state = client.getSwitchRunState()
    return {
        "run_state": int(run_state),
        "run_state_name": get_run_state_name(int(run_state)),
    }


def _check_boot_type(client) -> dict[str, Any]:
    boot_type = client.getBootType()
    return {
        "boot_type": int(boot_type),
        "boot_type_name": get_boot_type_name(int(boot_type)),
    }


def _check_lldp_neighbors(client) -> dict[str, Any]:
    neighbors = client.getLldpNeighbors()
    neighbor_list = [
        {"local_port": n.localPort, "remote_system": n.systemName} for n in neighbors
    ]
    return {
        "count": len(neighbors),
        "neighbors": neighbor_list[:10],
    }


def _check_arp_table(client) -> dict[str, Any]:
    arp_entries = client.getArpTable()
    return {"count": len(arp_entries)}


def _check_ndp_table(client) -> dict[str, Any]:
    ndp_entries = client.getNdpTable()
    return {"count": len(ndp_entries)}


def _check_route_table(client) -> dict[str, Any]:
    routes = client.getRouteTableDetails()
    return {"route_count": len(routes)}


def _check_product_info(client) -> dict[str, Any]:
    product_info = client.getProductInfo()
    return {
        "product": getattr(product_info, "product", None),
        "serial": getattr(product_info, "serial", None),
        "version": getattr(product_info, "version", None),
    }


def _check_config_info(client) -> dict[str, Any]:
    config_info = client.getConfigAppliedInfo()
    return {"last_config_applied_ms": getattr(config_info, "lastAppliedInMs", None)}


def _check_fabric_connectivity(client) -> dict[str, Any]:
    fabric = client.getFabricConnectivity()
    return {"endpoints": len(fabric)}


def _check_agent_status(client) -> dict[str, Any]:
    status = client.getHwAgentConnectionStatus()
    return {"agents": len(status)}


def _safe_check(check_fn, client) -> dict[str, Any]:
    try:
        return check_fn(client)
    except Exception as e:
        return {"error": str(e)}


def get_full_health_check(host: str, port: int = 5909) -> dict[str, Any]:
    health: dict[str, Any] = {
        "host": host,
        "port": port,
        "status": "unknown",
        "checks": {},
    }

    try:
        client, transport = create_client(host, port)

        health["checks"]["port_status"] = _check_port_status(client)
        health["checks"]["switch_state"] = _safe_check(_check_switch_state, client)
        health["checks"]["boot_type"] = _safe_check(_check_boot_type, client)
        health["checks"]["lldp_neighbors"] = _safe_check(_check_lldp_neighbors, client)
        health["checks"]["arp_table"] = _safe_check(_check_arp_table, client)
        health["checks"]["ndp_table"] = _safe_check(_check_ndp_table, client)
        health["checks"]["route_table"] = _safe_check(_check_route_table, client)
        health["checks"]["product_info"] = _safe_check(_check_product_info, client)
        health["checks"]["config_info"] = _safe_check(_check_config_info, client)
        health["checks"]["fabric_connectivity"] = _safe_check(
            _check_fabric_connectivity, client
        )
        health["checks"]["agent_status"] = _safe_check(_check_agent_status, client)

        health["status"] = "healthy"
        transport.close()

    except Exception as e:
        health["status"] = "error"
        health["error"] = str(e)

    return health


def get_run_state_name(state: int) -> str:
    states = {
        0: "UNINITIALIZED",
        1: "INITIALIZED",
        2: "CONFIGURED",
        3: "FIB_SYNCED",
        4: "EXITING",
    }
    return states.get(state, f"UNKNOWN({state})")


def get_boot_type_name(boot_type: int) -> str:
    types = {0: "COLD_BOOT", 1: "WARM_BOOT"}
    return types.get(boot_type, f"UNKNOWN({boot_type})")


HTML_PAGE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>FBOSS Switch Health Analytics</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            color: #eee;
            min-height: 100vh;
            padding: 20px;
        }
        .container { max-width: 1400px; margin: 0 auto; }
        h1 { text-align: center; margin-bottom: 30px; color: #4fc3f7; font-size: 2em; }
        h2 { color: #4fc3f7; margin-bottom: 15px; font-size: 1.3em; }
        h3 { color: #81d4fa; margin-bottom: 10px; }
        
        .input-section {
            background: rgba(22, 33, 62, 0.9);
            padding: 25px;
            border-radius: 15px;
            margin-bottom: 25px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.3);
        }
        .input-row { display: flex; gap: 10px; margin-bottom: 15px; flex-wrap: wrap; }
        input[type="text"] {
            flex: 1; min-width: 250px;
            padding: 12px 15px;
            border: 2px solid #0f3460;
            border-radius: 8px;
            background: #0f3460;
            color: #fff;
            font-size: 14px;
            transition: border-color 0.3s;
        }
        input[type="text"]:focus { border-color: #4fc3f7; outline: none; }
        input[type="text"]::placeholder { color: #666; }
        input[type="number"] {
            width: 100px;
            padding: 12px;
            border: 2px solid #0f3460;
            border-radius: 8px;
            background: #0f3460;
            color: #fff;
            font-size: 14px;
        }
        button {
            padding: 12px 24px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 14px;
            font-weight: 600;
            transition: all 0.3s;
        }
        .btn-primary { background: linear-gradient(135deg, #4fc3f7, #29b6f6); color: #1a1a2e; }
        .btn-primary:hover { transform: translateY(-2px); box-shadow: 0 4px 15px rgba(79,195,247,0.4); }
        .btn-add { background: linear-gradient(135deg, #66bb6a, #4caf50); color: #fff; }
        .btn-add:hover { transform: translateY(-2px); }
        .btn-secondary { background: #546e7a; color: #fff; }
        .btn-remove { background: #ef5350; color: #fff; padding: 8px 12px; font-size: 12px; }
        
        .switch-list { display: flex; flex-wrap: wrap; gap: 10px; margin: 15px 0; }
        .switch-chip {
            display: flex; align-items: center; gap: 8px;
            padding: 8px 12px;
            background: linear-gradient(135deg, #0f3460, #1a1a2e);
            border: 1px solid #4fc3f7;
            border-radius: 20px;
            font-size: 13px;
        }
        
        .analytics-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 25px;
        }
        .analytics-card {
            background: rgba(22, 33, 62, 0.9);
            border-radius: 15px;
            padding: 20px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.3);
        }
        .analytics-card.full-width { grid-column: 1 / -1; }
        
        .summary-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
            margin-bottom: 20px;
        }
        .summary-stat {
            background: linear-gradient(135deg, #0f3460, #1a1a2e);
            padding: 20px;
            border-radius: 12px;
            text-align: center;
            border: 1px solid rgba(79,195,247,0.2);
        }
        .summary-stat .value { font-size: 2.5em; font-weight: 700; color: #4fc3f7; }
        .summary-stat .label { font-size: 0.85em; color: #888; margin-top: 5px; }
        .summary-stat.success .value { color: #4caf50; }
        .summary-stat.warning .value { color: #ff9800; }
        .summary-stat.error .value { color: #f44336; }
        
        .switch-card {
            background: rgba(15, 52, 96, 0.5);
            border-radius: 12px;
            padding: 20px;
            margin-bottom: 15px;
            border-left: 4px solid #4caf50;
        }
        .switch-card.error { border-left-color: #f44336; }
        .switch-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 15px;
        }
        .switch-name { font-size: 1.1em; font-weight: 600; color: #4fc3f7; }
        .status-badge {
            padding: 5px 12px;
            border-radius: 15px;
            font-size: 11px;
            font-weight: 600;
            text-transform: uppercase;
        }
        .status-healthy { background: #4caf50; }
        .status-error { background: #f44336; }
        
        .metrics-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            gap: 10px;
        }
        .metric {
            background: rgba(0,0,0,0.2);
            padding: 12px;
            border-radius: 8px;
            text-align: center;
        }
        .metric .value { font-size: 1.5em; font-weight: 600; color: #81d4fa; }
        .metric .label { font-size: 0.75em; color: #888; margin-top: 3px; }
        
        .port-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(100px, 1fr));
            gap: 6px;
            max-height: 200px;
            overflow-y: auto;
            padding: 10px;
            background: rgba(0,0,0,0.2);
            border-radius: 8px;
        }
        .port-item {
            padding: 6px 10px;
            border-radius: 4px;
            font-size: 11px;
            display: flex;
            justify-content: space-between;
        }
        .port-up { background: rgba(76,175,80,0.3); border: 1px solid #4caf50; }
        .port-down { background: rgba(244,67,54,0.2); border: 1px solid #f44336; }
        
        .chart-container { position: relative; height: 250px; }
        .loading { text-align: center; padding: 60px; color: #888; font-size: 1.2em; }
        .error-msg { background: rgba(244,67,54,0.2); border: 1px solid #f44336; padding: 15px; border-radius: 8px; color: #f44336; }
        
        table { width: 100%; border-collapse: collapse; margin-top: 15px; }
        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #0f3460; }
        th { background: rgba(0,0,0,0.3); color: #4fc3f7; font-weight: 600; }
        tr:hover { background: rgba(79,195,247,0.1); }
        
        .tabs { display: flex; gap: 5px; margin-bottom: 20px; }
        .tab {
            padding: 10px 20px;
            background: #0f3460;
            border: none;
            border-radius: 8px 8px 0 0;
            color: #888;
            cursor: pointer;
            font-size: 14px;
        }
        .tab.active { background: #16213e; color: #4fc3f7; }
        .tab-content { display: none; }
        .tab-content.active { display: block; }
    </style>
</head>
<body>
    <div class="container">
        <h1>FBOSS Switch Health Analytics Dashboard</h1>
        
        <div class="input-section">
            <h2>Add Switches to Monitor</h2>
            <div class="input-row">
                <input type="text" id="switchName" placeholder="Enter switch hostname (e.g., susw001.r147.c081.dkl6)">
                <input type="number" id="switchPort" value="5909" min="1" max="65535">
                <button class="btn-add" onclick="addSwitch()">+ Add Switch</button>
            </div>
            <div class="switch-list" id="switchList"></div>
            <div class="input-row">
                <button class="btn-primary" onclick="runHealthCheck()">Run Full Health Check</button>
                <button class="btn-secondary" onclick="clearAll()">Clear All</button>
            </div>
        </div>
        
        <div id="results"></div>
    </div>

    <script>
        let switches = [];
        let healthResults = [];
        let charts = {};
        
        function addSwitch() {
            const name = document.getElementById('switchName').value.trim();
            const port = parseInt(document.getElementById('switchPort').value) || 5909;
            if (!name) { alert('Please enter hostname'); return; }
            if (switches.some(s => s.name === name)) { alert('Already added'); return; }
            switches.push({ name, port });
            document.getElementById('switchName').value = '';
            renderSwitchList();
        }
        
        function removeSwitch(i) { switches.splice(i, 1); renderSwitchList(); }
        
        function renderSwitchList() {
            const list = document.getElementById('switchList');
            list.innerHTML = switches.length === 0 
                ? '<p style="color:#666">No switches added</p>'
                : switches.map((s, i) => `
                    <div class="switch-chip">
                        <span>${s.name}:${s.port}</span>
                        <button class="btn-remove" onclick="removeSwitch(${i})">X</button>
                    </div>
                `).join('');
        }
        
        function clearAll() {
            switches = [];
            healthResults = [];
            renderSwitchList();
            document.getElementById('results').innerHTML = '';
        }
        
        async function runHealthCheck() {
            if (switches.length === 0) { alert('Add at least one switch'); return; }
            document.getElementById('results').innerHTML = '<div class="loading">Running comprehensive health checks...</div>';
            
            healthResults = [];
            for (const sw of switches) {
                try {
                    const resp = await fetch(`/api/full-health?host=${encodeURIComponent(sw.name)}&port=${sw.port}`);
                    healthResults.push(await resp.json());
                } catch (e) {
                    healthResults.push({ host: sw.name, port: sw.port, status: 'error', error: e.message, checks: {} });
                }
            }
            renderAnalytics();
        }
        
        function renderAnalytics() {
            const totalSwitches = healthResults.length;
            const healthySwitches = healthResults.filter(r => r.status === 'healthy').length;
            const totalPorts = healthResults.reduce((sum, r) => sum + (r.checks.port_status?.total_ports || 0), 0);
            const portsUp = healthResults.reduce((sum, r) => sum + (r.checks.port_status?.ports_up || 0), 0);
            const portsDown = healthResults.reduce((sum, r) => sum + (r.checks.port_status?.ports_down || 0), 0);
            const totalRoutes = healthResults.reduce((sum, r) => sum + (r.checks.route_table?.route_count || 0), 0);
            const totalLldp = healthResults.reduce((sum, r) => sum + (r.checks.lldp_neighbors?.count || 0), 0);
            
            let html = `
                <div class="analytics-card full-width">
                    <h2>Overall Summary</h2>
                    <div class="summary-grid">
                        <div class="summary-stat ${healthySwitches === totalSwitches ? 'success' : 'warning'}">
                            <div class="value">${healthySwitches}/${totalSwitches}</div>
                            <div class="label">Switches Healthy</div>
                        </div>
                        <div class="summary-stat ${portsUp > 0 ? 'success' : 'warning'}">
                            <div class="value">${portsUp}</div>
                            <div class="label">Ports Up</div>
                        </div>
                        <div class="summary-stat ${portsDown === 0 ? 'success' : 'error'}">
                            <div class="value">${portsDown}</div>
                            <div class="label">Ports Down</div>
                        </div>
                        <div class="summary-stat">
                            <div class="value">${totalRoutes}</div>
                            <div class="label">Total Routes</div>
                        </div>
                        <div class="summary-stat">
                            <div class="value">${totalLldp}</div>
                            <div class="label">LLDP Neighbors</div>
                        </div>
                    </div>
                </div>
                
                <div class="analytics-grid">
                    <div class="analytics-card">
                        <h3>Port Status Distribution</h3>
                        <div class="chart-container"><canvas id="portChart"></canvas></div>
                    </div>
                    <div class="analytics-card">
                        <h3>Routes per Switch</h3>
                        <div class="chart-container"><canvas id="routeChart"></canvas></div>
                    </div>
                </div>
                
                <div class="analytics-card full-width">
                    <h2>Switch Comparison</h2>
                    <table>
                        <tr>
                            <th>Switch</th>
                            <th>Status</th>
                            <th>Product</th>
                            <th>Serial</th>
                            <th>Ports Up/Down</th>
                            <th>Routes</th>
                            <th>LLDP</th>
                            <th>Boot Type</th>
                            <th>State</th>
                        </tr>
                        ${healthResults.map(r => `
                            <tr>
                                <td>${r.host}</td>
                                <td><span class="status-badge status-${r.status}">${r.status}</span></td>
                                <td>${r.checks.product_info?.product || '-'}</td>
                                <td>${r.checks.product_info?.serial || '-'}</td>
                                <td style="color:${r.checks.port_status?.ports_up > 0 ? '#4caf50' : '#f44336'}">${r.checks.port_status?.ports_up || 0} / ${r.checks.port_status?.ports_down || 0}</td>
                                <td>${r.checks.route_table?.route_count || '-'}</td>
                                <td>${r.checks.lldp_neighbors?.count || 0}</td>
                                <td>${r.checks.boot_type?.boot_type_name || '-'}</td>
                                <td>${r.checks.switch_state?.run_state_name || '-'}</td>
                            </tr>
                        `).join('')}
                    </table>
                </div>
                
                <div class="analytics-card full-width">
                    <h2>Detailed Switch Health</h2>
                    <div class="tabs">
                        ${healthResults.map((r, i) => `<button class="tab ${i===0?'active':''}" onclick="showTab(${i})">${r.host.split('.')[0]}</button>`).join('')}
                    </div>
                    ${healthResults.map((r, i) => `
                        <div class="tab-content ${i===0?'active':''}" id="tab${i}">
                            <div class="switch-card ${r.status}">
                                <div class="switch-header">
                                    <span class="switch-name">${r.host}:${r.port}</span>
                                    <span class="status-badge status-${r.status}">${r.status}</span>
                                </div>
                                ${r.status === 'error' ? `<div class="error-msg">${r.error}</div>` : `
                                    <div class="metrics-grid">
                                        <div class="metric"><div class="value">${r.checks.port_status?.total_ports || 0}</div><div class="label">Total Ports</div></div>
                                        <div class="metric" style="color:#4caf50"><div class="value">${r.checks.port_status?.ports_up || 0}</div><div class="label">Ports Up</div></div>
                                        <div class="metric" style="color:#f44336"><div class="value">${r.checks.port_status?.ports_down || 0}</div><div class="label">Ports Down</div></div>
                                        <div class="metric"><div class="value">${r.checks.route_table?.route_count || 0}</div><div class="label">Routes</div></div>
                                        <div class="metric"><div class="value">${r.checks.lldp_neighbors?.count || 0}</div><div class="label">LLDP</div></div>
                                        <div class="metric"><div class="value">${r.checks.arp_table?.count || 0}</div><div class="label">ARP</div></div>
                                        <div class="metric"><div class="value">${r.checks.ndp_table?.count || 0}</div><div class="label">NDP</div></div>
                                        <div class="metric"><div class="value">${r.checks.agent_status?.agents || 0}</div><div class="label">Agents</div></div>
                                    </div>
                                    <h3 style="margin-top:20px">Port Details</h3>
                                    <div class="port-grid">
                                        ${Object.entries(r.checks.port_status?.ports || {}).sort().map(([name, p]) => `
                                            <div class="port-item ${p.up ? 'port-up' : 'port-down'}">
                                                <span>${name}</span>
                                                <span>${p.up ? 'UP' : 'DN'}</span>
                                            </div>
                                        `).join('')}
                                    </div>
                                `}
                            </div>
                        </div>
                    `).join('')}
                </div>
            `;
            
            document.getElementById('results').innerHTML = html;
            renderCharts();
        }
        
        function showTab(index) {
            document.querySelectorAll('.tab').forEach((t, i) => t.classList.toggle('active', i === index));
            document.querySelectorAll('.tab-content').forEach((c, i) => c.classList.toggle('active', i === index));
        }
        
        function renderCharts() {
            if (charts.portChart) charts.portChart.destroy();
            if (charts.routeChart) charts.routeChart.destroy();
            
            const labels = healthResults.map(r => r.host.split('.')[0]);
            const portsUp = healthResults.map(r => r.checks.port_status?.ports_up || 0);
            const portsDown = healthResults.map(r => r.checks.port_status?.ports_down || 0);
            const routes = healthResults.map(r => r.checks.route_table?.route_count || 0);
            
            charts.portChart = new Chart(document.getElementById('portChart'), {
                type: 'bar',
                data: {
                    labels,
                    datasets: [
                        { label: 'Ports Up', data: portsUp, backgroundColor: '#4caf50' },
                        { label: 'Ports Down', data: portsDown, backgroundColor: '#f44336' }
                    ]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: { legend: { labels: { color: '#eee' } } },
                    scales: {
                        x: { stacked: true, ticks: { color: '#888' }, grid: { color: '#333' } },
                        y: { stacked: true, ticks: { color: '#888' }, grid: { color: '#333' } }
                    }
                }
            });
            
            charts.routeChart = new Chart(document.getElementById('routeChart'), {
                type: 'doughnut',
                data: {
                    labels,
                    datasets: [{ data: routes, backgroundColor: ['#4fc3f7', '#81d4fa', '#29b6f6', '#03a9f4', '#039be5'] }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: { legend: { position: 'right', labels: { color: '#eee' } } }
                }
            });
        }
        
        document.getElementById('switchName').addEventListener('keypress', e => { if (e.key === 'Enter') addSwitch(); });
        renderSwitchList();
    </script>
</body>
</html>
"""


class HealthCheckHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        pass

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)

        if parsed.path == "/" or parsed.path == "/index.html":
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode())

        elif parsed.path == "/api/full-health":
            params = urllib.parse.parse_qs(parsed.query)
            host = params.get("host", [""])[0]
            port = int(params.get("port", ["5909"])[0])

            if not host:
                self.send_response(400)
                self.send_header("Content-type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"error": "host required"}).encode())
                return

            health = get_full_health_check(host, port)
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(health, default=str).encode())

        else:
            self.send_response(404)
            self.end_headers()


def main():
    parser = argparse.ArgumentParser(description="FBOSS Switch Health Analytics UI")
    parser.add_argument("--port", type=int, default=8080, help="Web server port")
    parser.add_argument("--host", default="0.0.0.0", help="Web server host")
    args = parser.parse_args()

    with socketserver.TCPServer((args.host, args.port), HealthCheckHandler) as httpd:
        print(
            f"FBOSS Health Analytics Dashboard running at http://localhost:{args.port}"
        )
        print("Press Ctrl+C to stop")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down...")


if __name__ == "__main__":
    main()
