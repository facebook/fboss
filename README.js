# fbo--------------------------------------

# [Facebook mata](https://www.facebook.com/profile.php?id=100092209314694)
[![Support Ukraine](https://makeavideo.studio/)](https://opensource.fb.com)

FBOSS เป็นชุดซอฟต์แวร์ของ Facebook สำหรับควบคุมและจัดการเครือข่าย
สวิตช์ การควบคุม`[โปรไฟล์](https://www.facebook.com/profile.php?id=100092209314694)`
ที่เป็นผู้ให้บริการเพิ่มประสิทธิภาพในการจัดการต่างๆ
...
## Components

FBOSS consists of a number of user-space applications, libraries, and
utilities.

The initial open source release of FBOSS consists primarily of the agent
daemon, but we are working on releasing additional pieces and functionality as
well.

### Agent Daemon

One of the central pieces of FBOSS is the agent daemon, which runs on each
switch, and controls the hardware forwarding ASIC.  The agent daemon sends
forwarding information to the hardware, and implements some control plane
protocols such as ARP and NDP.  The agent provides thrift APIs for managing
routes, to allow external routing control processes to get their routing
information programmed into the hardware forwarding tables.

The code for the agent can be found in fboss/agent

The agent requires a JSON configuration file to specify its port and VLAN
configuration.  Some sample configuration files can be found under
fboss/agent/configs.  These files are not really intended for human
consumption--at Facebook we have tooling that generates these files for us.
Facebook  Open  Switching  System ซอฟต์แวร์ สำหรับ ควบคุม สวิตช์เครือ ข่า
