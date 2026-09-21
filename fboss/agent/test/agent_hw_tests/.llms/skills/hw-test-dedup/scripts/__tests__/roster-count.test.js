'use strict';

const assert = require('assert');
const { countTests } = require('../roster-count');

const fixture = `
TEST_F(MySuite, FirstTest) { EXPECT_TRUE(true); }
TEST_F(MySuite, SecondTest) { EXPECT_EQ(1, 1); }
TEST_F(OtherSuite, ThirdTest) {
  doStuff();
}
TEST_P(ParamSuite, FourthTest) { EXPECT_TRUE(true); }
`;

const { count, names, undetermined } = countTests(fixture);

assert.strictEqual(count, 4, `expected count 4, got ${count}`);
assert.ok(names.includes('FirstTest'), 'expected FirstTest in names');
assert.ok(names.includes('FourthTest'), 'expected FourthTest in names');
assert.strictEqual(undetermined.length, 1, 'expected 1 undetermined (TEST_P)');
assert.strictEqual(undetermined[0].macro, 'TEST_P');

console.log('roster-count OK');
process.exit(0);
