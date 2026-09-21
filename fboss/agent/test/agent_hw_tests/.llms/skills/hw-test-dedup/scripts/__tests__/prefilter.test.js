'use strict';

const assert = require('assert');
const { prunePairs } = require('../prefilter');

const tests = [
  {
    id: 'A',
    fixtureBase: 'PortFixture',
    featureArea: 'port',
    productionFeatures: ['L2', 'L3'],
    asics: ['TH4'],
    helperSymbols: ['setupPort'],
    nameTokens: ['port', 'up'],
  },
  {
    id: 'B',
    fixtureBase: 'PortFixture',
    featureArea: 'port',
    productionFeatures: ['L3', 'QOS'],
    asics: ['TH4'],
    helperSymbols: ['setupPort'],
    nameTokens: ['port', 'down'],
  },
  {
    id: 'C',
    fixtureBase: 'PortFixture',
    featureArea: 'port',
    productionFeatures: ['MPLS'],
    asics: ['TH4'],
    helperSymbols: ['setupPort'],
    nameTokens: ['port', 'flap'],
  },
];

const { pairs, pruned } = prunePairs(tests);

assert.strictEqual(pairs.length, 1, `expected 1 surviving pair, got ${pairs.length}`);
const surviving = pairs[0];
const survivingIds = [surviving.a, surviving.b].sort().join('-');
assert.strictEqual(survivingIds, 'A-B', `expected surviving pair A-B, got ${survivingIds}`);

function isPruned(x, y) {
  return pruned.some(
    (p) => [p.a, p.b].sort().join('-') === [x, y].sort().join('-')
  );
}

assert.ok(isPruned('A', 'C'), 'expected A-C in pruned');
assert.ok(isPruned('B', 'C'), 'expected B-C in pruned');

console.log('prefilter OK');
process.exit(0);
