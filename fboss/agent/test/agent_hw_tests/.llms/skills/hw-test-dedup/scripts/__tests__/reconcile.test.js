const assert = require('assert');
const { reconcile, dagSelect } = require('../reconcile.js');

// Fingerprints
const byId = {
  // VerifyUdf-like subset; clean superset WithOtherAcls (same fixture, same UDF acls), refuted superset MinusUdfHash (adds hash -> less similar)
  S: { fixture: 'Udf', fixtureMode: 'single', productionFeatures: ['UDF'], matchTuples: ['udf_ack', 'udf_wr'], actions: ['counter'], counterTypes: ['p'], trafficMode: 'frontpanel', warmbootShape: '2arg' },
  WithOther: { fixture: 'Udf', fixtureMode: 'single', productionFeatures: ['UDF'], matchTuples: ['udf_ack', 'udf_wr', 'srcport'], actions: ['counter'], counterTypes: ['p'], trafficMode: 'frontpanel', warmbootShape: '2arg' },
  MinusHash: { fixture: 'Udf', fixtureMode: 'single', productionFeatures: ['UDF'], matchTuples: ['udf_ack', 'udf_wr', 'hash'], actions: ['counter', 'hash'], counterTypes: ['p'], trafficMode: 'frontpanel', warmbootShape: '4arg' },
  // AclStatCreate-like subset; confirmed superset StatNoOp (far: no-op count test), refuted superset Multiple (closer: creates stats)
  T: { fixture: 'Stat', fixtureMode: 'single', productionFeatures: ['STAT'], matchTuples: ['dscp'], actions: ['counter'], counterTypes: ['p'], trafficMode: 'none', warmbootShape: '2arg' },
  StatNoOp: { fixture: 'Stat', fixtureMode: 'single', productionFeatures: ['STAT'], matchTuples: ['count_noop'], actions: [], counterTypes: [], trafficMode: 'none', warmbootShape: '2arg' },
  Multiple: { fixture: 'Stat', fixtureMode: 'single', productionFeatures: ['STAT'], matchTuples: ['dscp'], actions: ['counter'], counterTypes: ['p'], trafficMode: 'none', warmbootShape: '2arg' },
};

const verified = [
  { candidate: { subsetId: 'S', supersetIds: ['WithOther'], relation: 'subset' }, status: 'confirmed' },
  { candidate: { subsetId: 'S', supersetIds: ['MinusHash'], relation: 'subset' }, status: 'needs_human_review' },
  { candidate: { subsetId: 'T', supersetIds: ['StatNoOp'], relation: 'subset' }, status: 'confirmed' },
  { candidate: { subsetId: 'T', supersetIds: ['Multiple'], relation: 'subset' }, status: 'needs_human_review' },
];

const { removable, conflictReview } = reconcile(verified, byId);
const removeIds = removable.map((r) => r.subsetId).sort();
const conflictIds = conflictReview.map((c) => c.subset).sort();

// S: clean WithOther is closer than refuted MinusHash -> removable
// T: confirmed StatNoOp is FARTHER than refuted Multiple -> conflict-veto -> review (NOT removed)
assert.deepStrictEqual(removeIds, ['S'], `expected only S removable, got ${removeIds}`);
assert.deepStrictEqual(conflictIds, ['T'], `expected only T in conflictReview, got ${conflictIds}`);

// S folds into the closest superset (WithOther), not MinusHash
assert.deepStrictEqual(removable[0].mergeTargets, ['WithOther']);

const finals = dagSelect(removable);
assert.strictEqual(finals.length, 1);
assert.strictEqual(finals[0].removeId, 'S');

console.log('reconcile OK');
process.exit(0);
