'use strict';

const MACROS = ['TEST_F', 'TEST_P', 'TYPED_TEST_P', 'TYPED_TEST'];

// TEST_P / TYPED_TEST families expand at runtime via INSTANTIATE_*, so their
// instantiation count is not statically determinable from the macro alone.
const NON_DETERMINISTIC = new Set(['TEST_P', 'TYPED_TEST', 'TYPED_TEST_P']);

function parseArgs(text, openParenIdx) {
  let depth = 0;
  let cur = '';
  const args = [];
  for (let i = openParenIdx; i < text.length; i++) {
    const ch = text[i];
    if (ch === '(') {
      depth++;
      if (depth === 1) continue;
    } else if (ch === ')') {
      depth--;
      if (depth === 0) {
        args.push(cur.trim());
        return args;
      }
    } else if (ch === ',' && depth === 1) {
      args.push(cur.trim());
      cur = '';
      continue;
    }
    cur += ch;
  }
  return null;
}

function countTests(fileText) {
  const names = [];
  const undetermined = [];
  let count = 0;

  for (const macro of MACROS) {
    const re = new RegExp('\\b' + macro + '\\s*\\(', 'g');
    let m;
    while ((m = re.exec(fileText)) !== null) {
      const openParen = fileText.indexOf('(', m.index);
      const args = parseArgs(fileText, openParen);
      if (!args || args.length < 2) continue;

      const suite = args[0];
      const testName = args[1];

      count++;
      names.push(testName);
      if (NON_DETERMINISTIC.has(macro)) {
        undetermined.push({ macro, suite });
      }
    }
  }

  return { count, names, undetermined };
}

module.exports = { countTests };
