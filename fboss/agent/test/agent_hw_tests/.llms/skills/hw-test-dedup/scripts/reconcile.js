// Per-subset reconciliation: pick the CLOSEST confirmed superset, and VETO a
// removal when a CLOSER superset was refuted (the most-similar test found unique
// coverage -> defer to human review rather than remove via a weaker superset).
// This is the tested reference implementation; dedup-workflow.js inlines the same
// logic (the Workflow sandbox cannot require() modules).

function jaccard(a, b) {
  const A = new Set(a || []);
  const B = new Set(b || []);
  if (A.size === 0 && B.size === 0) return 0;
  let inter = 0;
  for (const x of A) if (B.has(x)) inter++;
  return inter / (A.size + B.size - inter);
}

// closeness between a subset fingerprint and a superset fingerprint (higher = more similar)
function closenessOne(sub, sup) {
  if (!sub || !sup) return 0;
  let s = 0;
  if (sub.fixture && sub.fixture === sup.fixture) s += 3;
  if (sub.fixtureMode && sub.fixtureMode === sup.fixtureMode) s += 1;
  s += 2 * jaccard(sub.productionFeatures, sup.productionFeatures);
  s += 2 * jaccard(sub.matchTuples, sup.matchTuples);
  s += jaccard(sub.actions, sup.actions);
  s += jaccard(sub.counterTypes, sup.counterTypes);
  if (sub.warmbootShape && sub.warmbootShape === sup.warmbootShape) s += 1;
  if (sub.trafficMode && sub.trafficMode === sup.trafficMode) s += 1;
  return s;
}

// candidate closeness = mean over its supersets (union has >1)
function candidateCloseness(cand, byId) {
  const sub = byId[cand.subsetId];
  const sups = (cand.supersetIds || []).map((id) => byId[id]);
  if (!sups.length) return 0;
  return sups.reduce((acc, sp) => acc + closenessOne(sub, sp), 0) / sups.length;
}

// verified: [{ candidate, status: 'confirmed'|'needs_human_review' }], byId: globalId -> fingerprint
// returns { removable: [{subsetId, mergeTargets, relation}], conflictReview: [{subset, reason}] }
function reconcile(verified, byId) {
  const bySub = {};
  for (const v of verified) (bySub[v.candidate.subsetId] = bySub[v.candidate.subsetId] || []).push(v);

  const removable = [];
  const conflictReview = [];
  for (const [sid, vs] of Object.entries(bySub)) {
    const confirmed = vs.filter((v) => v.status === 'confirmed');
    const refuted = vs.filter((v) => v.status === 'needs_human_review');
    if (!confirmed.length) continue; // not removable here; surfaced via needs_human_review elsewhere
    const scored = confirmed
      .map((v) => ({ v, score: candidateCloseness(v.candidate, byId) }))
      .sort((a, b) => b.score - a.score);
    const best = scored[0];
    const refutedMax = refuted.length
      ? Math.max(...refuted.map((v) => candidateCloseness(v.candidate, byId)))
      : -1;
    if (best.score >= refutedMax) {
      removable.push({ subsetId: sid, mergeTargets: best.v.candidate.supersetIds, relation: best.v.candidate.relation });
    } else {
      conflictReview.push({ subset: sid, reason: `closest confirmed superset (score ${best.score.toFixed(2)}) is less similar than a refuted superset (score ${refutedMax.toFixed(2)}); deferring to human review` });
    }
  }
  return { removable, conflictReview };
}

// keep a maximal antichain: never remove a node whose only superset is itself removed
function dagSelect(removable) {
  const removeSet = new Set(removable.map((r) => r.subsetId));
  const out = [];
  for (const r of removable) {
    const sups = r.mergeTargets || [];
    const kept = r.relation === 'union'
      ? (sups.every((x) => !removeSet.has(x)) ? sups : null)
      : (sups.filter((x) => !removeSet.has(x)).length ? sups.filter((x) => !removeSet.has(x)) : null);
    if (kept) out.push({ removeId: r.subsetId, mergeTargets: kept, relation: r.relation });
  }
  out.sort((a, b) => a.removeId.localeCompare(b.removeId));
  const seen = new Set();
  return out.filter((r) => (seen.has(r.removeId) ? false : (seen.add(r.removeId), true)));
}

module.exports = { jaccard, closenessOne, candidateCloseness, reconcile, dagSelect };
