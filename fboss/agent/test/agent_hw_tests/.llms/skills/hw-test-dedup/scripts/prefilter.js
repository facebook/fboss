'use strict';

function hasField(meta, key) {
  return meta[key] !== undefined && meta[key] !== null && meta[key] !== '';
}

function clusterFiles(fileMetas) {
  const groups = new Map();
  const clusters = [];

  for (const meta of fileMetas) {
    if (!hasField(meta, 'fixtureBase') || !hasField(meta, 'featureArea')) {
      const missing = [];
      if (!hasField(meta, 'fixtureBase')) missing.push('fixtureBase');
      if (!hasField(meta, 'featureArea')) missing.push('featureArea');
      clusters.push({
        key: null,
        singleton: true,
        reason: `missing ${missing.join(',')}`,
        files: [meta.file],
      });
      continue;
    }

    const key = `${meta.fixtureBase}|${meta.featureArea}`;
    if (!groups.has(key)) {
      const cluster = { key, files: [] };
      groups.set(key, cluster);
      clusters.push(cluster);
    }
    groups.get(key).files.push(meta.file);
  }

  return clusters;
}

function overlaps(a, b) {
  if (!Array.isArray(a) || !Array.isArray(b)) return false;
  const set = new Set(a);
  return b.some((x) => set.has(x));
}

function prunePairs(tests) {
  const clusters = new Map();
  for (const t of tests) {
    const key = `${t.fixtureBase}|${t.featureArea}`;
    if (!clusters.has(key)) clusters.set(key, []);
    clusters.get(key).push(t);
  }

  const pairs = [];
  const pruned = [];

  for (const group of clusters.values()) {
    for (let i = 0; i < group.length; i++) {
      for (let j = i + 1; j < group.length; j++) {
        const a = group[i];
        const b = group[j];

        if (a.fixtureBase !== b.fixtureBase) {
          pruned.push({ a: a.id, b: b.id, reason: 'fixtureBase mismatch' });
          continue;
        }
        if (a.featureArea !== b.featureArea) {
          pruned.push({ a: a.id, b: b.id, reason: 'featureArea mismatch' });
          continue;
        }
        if (!overlaps(a.helperSymbols, b.helperSymbols) &&
            !overlaps(a.nameTokens, b.nameTokens)) {
          pruned.push({ a: a.id, b: b.id, reason: 'no helper/name overlap' });
          continue;
        }
        // productionFeatures is assumed pre-unioned across flag branches by the
        // caller, so disjointness here is a genuine signal the tests differ.
        if (!overlaps(a.productionFeatures, b.productionFeatures)) {
          pruned.push({ a: a.id, b: b.id, reason: 'disjoint productionFeatures' });
          continue;
        }
        if (!overlaps(a.asics, b.asics)) {
          pruned.push({ a: a.id, b: b.id, reason: 'disjoint asics' });
          continue;
        }
        pairs.push({ a: a.id, b: b.id });
      }
    }
  }

  return { pairs, pruned };
}

module.exports = { clusterFiles, prunePairs };
