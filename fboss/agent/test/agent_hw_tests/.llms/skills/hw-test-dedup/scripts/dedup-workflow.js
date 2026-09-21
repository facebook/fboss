export const meta = {
  name: 'hw-test-dedup',
  description: 'Find redundant FBOSS agent_hw_tests (coverage subsets) across a set of files and report safe removals; optionally apply as draft diffs',
  phases: [
    { title: 'Fingerprint' },
    { title: 'Detect' },
    { title: 'Refute' },
    { title: 'MergePlan' },
    { title: 'Report' },
  ],
}

// args: { skillDir, workDir, files:[abs...], area="acl", mode="report"|"apply" }
// Fingerprints are passed INLINE between phases (n is small per cluster); this mirrors
// the proven ACL workflow. Agents Read the reference docs for personas/gates/pack.
const A = args || {}
const SKILL = A.skillDir || '.'
const WORK = A.workDir || '/tmp/hw-test-dedup'
const FILES = A.files || []
const AREA = A.area || 'acl'
const MODE = A.mode === 'apply' ? 'apply' : 'report'
const REF = (p) => `${SKILL}/references/${p}`
const PACK = `${SKILL}/references/coverage-packs/${AREA}.md`

// Area-specific subsumption rules are NOT hardcoded here — they live in the
// active coverage pack (e.g. the CPU-vs-front-panel rule lives in coverage-packs/acl.md).
// The core only tells every agent to load and apply the pack authoritatively.
const PACK_RULES =
  `Apply EVERY area-specific subsumption/redundancy rule and extra gate predicate defined in the active coverage pack (${PACK}) as authoritative, in addition to the 6 universal gates. If the pack is missing or silent on a dimension, treat that dimension conservatively (do NOT subsume).`

if (!FILES.length) { log('No input files.'); return { error: 'no input files' } }
log(`hw-test-dedup: ${FILES.length} file(s), area=${AREA}, mode=${MODE}`)

const FP_SCHEMA = {
  type: 'object',
  required: ['rosterCount', 'tests'],
  properties: {
    rosterCount: { type: 'number', description: 'macro count from scripts/roster-count.js for this file' },
    tests: {
      type: 'array',
      items: {
        type: 'object',
        required: ['globalId', 'testName', 'fixture', 'productionFeatures'],
        properties: {
          globalId: { type: 'string' }, testName: { type: 'string' },
          fixture: { type: 'string' }, baseClass: { type: 'string' },
          productionFeatures: { type: 'array', items: { type: 'string' } },
          fixtureMode: { type: 'string', description: 'area-agnostic `<binding>:<modeTokens>` (binding pinned|inherited); mode tokens are area-specific, defined by the coverage pack (e.g. ACL pinned:multi)' },
          asics: { type: 'array', items: { type: 'string' } },
          matchTuples: { type: 'array', items: { type: 'string' } },
          actions: { type: 'array', items: { type: 'string' } },
          counterTypes: { type: 'array', items: { type: 'string' } },
          hwObjects: { type: 'array', items: { type: 'string' } },
          verificationApis: { type: 'array', items: { type: 'string' } },
          warmbootShape: { type: 'object', properties: { argForm: { type: 'string' }, coldSetup: { type: 'array', items: { type: 'string' } }, coldVerify: { type: 'array', items: { type: 'string' } }, postWbSetup: { type: 'array', items: { type: 'string' } }, postWbVerify: { type: 'array', items: { type: 'string' } }, direction: { type: 'string' } } },
          trafficMode: { type: 'string' },
          addrFamily: { type: 'string' },
          asicSdkGates: { type: 'array', items: { type: 'string' } },
          polarity: { type: 'string' },
          mutationSequence: { type: 'array', items: { type: 'string' } },
          scaleClass: { type: 'string' },
          coveragePack: { type: 'object' },
          uniqueAssertions: { type: 'array', items: { type: 'string' } },
        },
      },
    },
  },
}
const CAND_SCHEMA = { type: 'object', required: ['candidates'], properties: { candidates: { type: 'array', items: { type: 'object', required: ['subsetId', 'supersetIds', 'relation', 'rationale'], properties: { subsetId: { type: 'string' }, supersetIds: { type: 'array', items: { type: 'string' } }, relation: { type: 'string', enum: ['subset', 'equivalent', 'union'] }, rationale: { type: 'string' }, uniqueBitsInSubset: { type: 'array', items: { type: 'string' } }, gatesPassed: { type: 'string' } } } } } }
const VERDICT_SCHEMA = { type: 'object', required: ['verdict', 'confidence', 'lens'], properties: { verdict: { type: 'string', enum: ['safe', 'unsafe'] }, hwBehaviorLost: { type: ['string', 'null'] }, confidence: { type: 'number' }, lens: { type: 'string' } } }
const MERGE_SCHEMA = { type: 'object', required: ['removable', 'portabilityOk'], properties: { removable: { type: 'boolean' }, portabilityOk: { type: 'boolean' }, mergeItems: { type: 'array', items: { type: 'string' } }, reason: { type: 'string' } } }

// ---- Phase 1: fingerprint (one agent per file, full fingerprints inline) ----
phase('Fingerprint')
const fpResults = await parallel(FILES.map((file) => () =>
  agent(
    `You are the Fingerprinter for hw-test-dedup. Read first: ${REF('personas.md')} (Fingerprinter), ${REF('fingerprint-schema.md')}, ${REF('universal-gates.md')}, ${PACK}, ${REF('warmboot-semantics.md')}.
TARGET FILE: ${file}. Read it IN FULL and transitively resolve helpers. For EVERY test macro (TEST_F/TEST_P/TYPED_TEST) emit exactly one fingerprint (globalId="${file}::TestName") — collapse parameterized/typed instantiations into that single fingerprint and union their coverage, so the count matches scripts/roster-count.js — with the ${AREA} coveragePack fields, captured well enough that another agent can judge coverage containment WITHOUT re-reading the file.
Also run the deterministic counter and report it as rosterCount:
  node -e "const{countTests}=require('${SKILL}/scripts/roster-count.js');const fs=require('fs');console.log(countTests(fs.readFileSync('${file}','utf8')).count)"
Return { rosterCount, tests:[...] }.`,
    { label: `fp:${file.split('/').pop()}`, phase: 'Fingerprint', schema: FP_SCHEMA, agentType: 'meta_codesearch:code-search' }
  )
))
const clean = fpResults.filter(Boolean)
const allTests = clean.flatMap((r) => r.tests || [])
// Hard completeness gate: a dropped test could be the unique owner of a behavior
// another test is then judged redundant against. Abort rather than dedup blind.
const completenessMismatches = clean
  .filter((r) => typeof r.rosterCount !== 'number' || r.rosterCount !== (r.tests || []).length)
  .map((r) => ({ file: (r.tests && r.tests[0] && r.tests[0].globalId || '?').split('::')[0], rosterCount: r.rosterCount, fingerprints: (r.tests || []).length }))
if (completenessMismatches.length) {
  log(`COMPLETENESS GATE FAILED: ${completenessMismatches.map((m) => `${m.file.split('/').pop()} roster ${m.rosterCount} != ${m.fingerprints}`).join('; ')}. Aborting — fix fingerprint coverage before dedup.`)
  return { error: 'completeness gate failed', completenessMismatches }
}
log(`Fingerprinted ${allTests.length} test cases; completeness gate passed.`)

// ---- deterministic: feature owners + last-owner ----
const featureOwners = {}
for (const t of allTests) for (const f of (t.productionFeatures || [])) (featureOwners[f] = featureOwners[f] || []).push(t.globalId)
const lastOwner = new Set()
for (const [, owners] of Object.entries(featureOwners)) if (owners.length === 1) lastOwner.add(owners[0])
log(`Last-feature-owner (non-removable): ${lastOwner.size}`)

// ---- deterministic: soft pre-filter — keep a pair iff it shares >=1 production feature ----
function shared(a, b) { const s = new Set(a || []); return (b || []).some((x) => s.has(x)) }
const ids = allTests.map((t) => t.globalId)
const byId = Object.fromEntries(allTests.map((t) => [t.globalId, t]))
const pruned = []
const keepPair = new Set()
for (let i = 0; i < ids.length; i++) for (let j = i + 1; j < ids.length; j++) {
  const a = byId[ids[i]], b = byId[ids[j]]
  if (shared(a.productionFeatures, b.productionFeatures)) keepPair.add([ids[i], ids[j]].sort().join('|'))
  else pruned.push({ pair: [ids[i], ids[j]], reason: 'no shared production feature' })
}
log(`Pre-filter: ${keepPair.size} pairs kept, ${pruned.length} pruned (no shared feature).`)

// ---- Phase 2: detect (anchor-sharded, full fingerprints inline) ----
phase('Detect')
const rosterJson = JSON.stringify(allTests)
const NSHARD = Math.min(5, Math.max(1, Math.ceil(ids.length / 14)))
const shards = Array.from({ length: NSHARD }, () => [])
ids.forEach((id, i) => shards[i % NSHARD].push(id))
const detRes = await parallel(shards.map((anchorIds, si) => () =>
  agent(
    `You are the Subset-detector for hw-test-dedup. Read ${REF('universal-gates.md')} (6 gates) and ${PACK} (${AREA} pack). ${PACK_RULES}

Consider tests in YOUR ANCHOR LIST as candidate SUPERSETS: ${JSON.stringify(anchorIds)}.
Find every OTHER test whose HW coverage is fully contained in an anchor (relation subset/equivalent), or jointly by an anchor + others (relation union). Apply ALL 6 universal gates + the ${AREA} pack's predicates and subsumption rules. Be conservative: if a gate is uncertain, do NOT emit. Missing ASSERTIONS in the superset don't disqualify (merge later); missing HW COVERAGE does. Consider same-file AND cross-file pairs.

FULL FINGERPRINTS (JSON):
${rosterJson}

Return all candidates {subsetId, supersetIds, relation, rationale, uniqueBitsInSubset, gatesPassed}.`,
    { label: `detect:shard${si}`, phase: 'Detect', schema: CAND_SCHEMA }
  )
))
let candidates = []
for (const r of detRes.filter(Boolean)) for (const c of (r.candidates || [])) candidates.push(c)
const droppedLastOwner = candidates.filter((c) => lastOwner.has(c.subsetId)).map((c) => c.subsetId)
candidates = candidates.filter((c) => !lastOwner.has(c.subsetId))
const seen = new Set()
candidates = candidates.filter((c) => { const k = `${c.subsetId}|${c.relation}|${[...(c.supersetIds || [])].sort().join(',')}`; if (seen.has(k)) return false; seen.add(k); return true })
log(`Detect: ${candidates.length} candidates (${droppedLastOwner.length} dropped last-owner).`)

// ---- Phase 3: refute (3 diverse lenses, unanimous-safe) ----
phase('Refute')
const LENSES = [
  { key: 'lifecycle', focus: 'warmboot shape & lifecycle/mutation ordering' },
  { key: 'asic_mode', focus: 'ASIC/SDK gating, matched-condition value-classes, fixture-mode parity (binding + mode set), and every area-specific value-class / subsumption rule defined in the active coverage pack' },
  { key: 'verif_semantics', focus: 'verification depth per shared object, polarity/negative, priority/scale, feature attestation, and the active coverage pack rules' },
]
const verified = await pipeline(candidates,
  (c) => parallel(LENSES.map((lens) => () =>
    agent(
      `Adversarial Skeptic (lens ${lens.key}) for hw-test-dedup. Read ${REF('universal-gates.md')} and the active coverage pack (${PACK}). ${PACK_RULES}
Try to REFUTE removing the subset; default UNSAFE if uncertain.
  subset: ${c.subsetId}
  superset(s): ${JSON.stringify(c.supersetIds)}
  relation: ${c.relation}
  rationale: ${c.rationale}
LENS focus: ${lens.focus}
${lens.key === 'verif_semantics' ? 'READ THE RAW SOURCE of subset+superset (and helpers) with search_files/Read.' : 'Use the rationale; read source if ambiguous.'}
Would removing the subset lose ANY HW coverage on ANY (ASIC,SDK,mode), under your lens AND the active coverage pack's rules? If yes -> unsafe + hwBehaviorLost. Else -> safe.`,
      { label: `refute:${lens.key}:${c.subsetId.split('::').pop()}`, phase: 'Refute', schema: VERDICT_SCHEMA, agentType: lens.key === 'verif_semantics' ? 'meta_codesearch:code-search' : undefined }
    )
  )).then((vs) => { const v = vs.filter(Boolean); const ok = v.length === LENSES.length && v.every((x) => x.verdict === 'safe'); return { candidate: c, verdicts: v, status: ok ? 'confirmed' : 'needs_human_review', refutations: v.filter((x) => x.verdict === 'unsafe') } })
)
const vClean = verified.filter(Boolean)
const confirmed = vClean.filter((v) => v.status === 'confirmed')
const humanReview = vClean.filter((v) => v.status === 'needs_human_review')
log(`Refute: ${confirmed.length} confirmed, ${humanReview.length} human-review.`)

// ---- deterministic: per-subset reconciliation (mirror of scripts/reconcile.js) ----
// Pick the CLOSEST confirmed superset per subset; VETO the removal (-> human review)
// when a CLOSER superset was refuted (the most-similar test found unique coverage).
function jaccard(a, b) { const A = new Set(a || []), B = new Set(b || []); if (!A.size && !B.size) return 0; let i = 0; for (const x of A) if (B.has(x)) i++; return i / (A.size + B.size - i) }
function closenessOne(sub, sup) { if (!sub || !sup) return 0; let s = 0; if (sub.fixture && sub.fixture === sup.fixture) s += 3; if (sub.fixtureMode && sub.fixtureMode === sup.fixtureMode) s += 1; s += 2 * jaccard(sub.productionFeatures, sup.productionFeatures); s += 2 * jaccard(sub.matchTuples, sup.matchTuples); s += jaccard(sub.actions, sup.actions); s += jaccard(sub.counterTypes, sup.counterTypes); if (sub.warmbootShape && sub.warmbootShape === sup.warmbootShape) s += 1; if (sub.trafficMode && sub.trafficMode === sup.trafficMode) s += 1; return s }
function candCloseness(c) { const sub = byId[c.subsetId]; const sups = (c.supersetIds || []).map((id) => byId[id]); if (!sups.length) return 0; return sups.reduce((a, sp) => a + closenessOne(sub, sp), 0) / sups.length }
const bySub = {}
for (const v of vClean) (bySub[v.candidate.subsetId] = bySub[v.candidate.subsetId] || []).push(v)
const removable = []
const conflictReview = []
for (const [sid, vs] of Object.entries(bySub)) {
  const conf = vs.filter((v) => v.status === 'confirmed')
  const ref = vs.filter((v) => v.status === 'needs_human_review')
  if (!conf.length) continue
  const best = conf.map((v) => ({ v, score: candCloseness(v.candidate) })).sort((a, b) => b.score - a.score)[0]
  const refMax = ref.length ? Math.max(...ref.map((v) => candCloseness(v.candidate))) : -1
  if (best.score >= refMax) removable.push({ subsetId: sid, mergeTargets: best.v.candidate.supersetIds, relation: best.v.candidate.relation })
  else conflictReview.push({ subset: sid, reason: `closest confirmed superset (score ${best.score.toFixed(2)}) less similar than a refuted superset (score ${refMax.toFixed(2)}); deferring to human review` })
}
const removeSet = new Set(removable.map((r) => r.subsetId))
function keptSup(r) { const s = r.mergeTargets || []; if (r.relation === 'union') return s.every((x) => !removeSet.has(x)) ? s : null; const k = s.filter((x) => !removeSet.has(x)); return k.length ? k : null }
const rraw = []
for (const r of removable) { const k = keptSup(r); if (k) rraw.push({ removeId: r.subsetId, mergeTargets: k, relation: r.relation }) }
rraw.sort((a, b) => a.removeId.localeCompare(b.removeId))
const dseen = new Set()
const removals = rraw.filter((r) => (dseen.has(r.removeId) ? false : (dseen.add(r.removeId), true)))
log(`Reconcile: ${removals.length} removable, ${conflictReview.length} conflict-vetoed to human review.`)

// ---- Phase 4: merge-plan ----
phase('MergePlan')
const merges = await parallel(removals.map((r) => () =>
  agent(
    `Merge-planner for hw-test-dedup. Read ${REF('personas.md')} (Merge-planner) and the active coverage pack (${PACK}). ${PACK_RULES}
remove: ${r.removeId}
merge into: ${JSON.stringify(r.mergeTargets)}
relation: ${r.relation}
Read the removed test + target(s) (and helpers) via search_files/Read. List exact unique assertions to port first (file:line). Portability: can each port without changing the target's own coverage? If not, removable=false + reason.`,
    { label: `merge:${r.removeId.split('::').pop()}`, phase: 'MergePlan', schema: MERGE_SCHEMA, agentType: 'meta_codesearch:code-search' }
  ).then((m) => ({ removal: r, merge: m }))
))
const mClean = merges.filter(Boolean)
const finalRemovals = mClean.filter((m) => m.merge && m.merge.removable && m.merge.portabilityOk)
const downgraded = mClean.filter((m) => !(m.merge && m.merge.removable && m.merge.portabilityOk))
log(`Merge: ${finalRemovals.length} final, ${downgraded.length} downgraded.`)

// ---- Phase 5: report + machine-readable dispositions ----
phase('Report')
const reportData = {
  mode: MODE, area: AREA, totalAnalyzed: allTests.length,
  finalRemovals: finalRemovals.map((m) => ({ remove: m.removal.removeId, mergeInto: m.removal.mergeTargets, relation: m.removal.relation, mergeItems: m.merge.mergeItems })),
  downgraded: downgraded.map((m) => ({ id: m.removal.removeId, reason: m.merge && m.merge.reason })),
  needsHumanReview: [
    ...humanReview.map((v) => ({ subset: v.candidate.subsetId, superset: v.candidate.supersetIds, refutations: v.refutations.map((x) => ({ lens: x.lens, lost: x.hwBehaviorLost })) })),
    ...conflictReview.map((c) => ({ subset: c.subset, conflictVeto: c.reason })),
  ],
  nonRemovableLastOwners: [...lastOwner], droppedLastOwnerCandidates: droppedLastOwner,
  prunedByPrefilter: pruned.length, featureOwners,
}
const reportPath = `${WORK}/HW_TEST_DEDUP_REPORT.md`
const dispPath = `${WORK}/dispositions.json`
const summary = await agent(
  `Synthesizer for hw-test-dedup. (1) mkdir -p ${WORK}. (2) Write the machine-readable dispositions to ${dispPath} as exactly this JSON (use Write):
${JSON.stringify(reportData)}
(3) Write a readable markdown report to ${reportPath} with: executive summary, removable table (remove, merge-into, relation), merge checklist, coverage-gap audit, non-removable floor (last-owners from featureOwners len==1), needs-human-review, downgraded, pruned count, next steps. Use verbatim test names. Return an 8-line headline.`,
  { label: 'report', phase: 'Report' }
)

return { reportPath, dispPath, summary, counts: { totalAnalyzed: allTests.length, candidates: candidates.length, confirmed: confirmed.length, finalRemovals: finalRemovals.length, humanReview: humanReview.length + conflictReview.length, conflictVetoed: conflictReview.length, pruned: pruned.length } }
