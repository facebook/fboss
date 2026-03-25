/**
 * (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
 */

/*
 *  NOTE: This will only work if building documentation from a git repo.
 *  Meta employees building this internally will see errors and an empty
 *  changelog.
 */

const path = require('path');
const {execSync} = require('child_process');
const {normalizeUrl} = require('@docusaurus/utils');

/**
 * @param {import('@docusaurus/types').LoadContext} context
 * @returns {import('@docusaurus/types').Plugin}
 */
module.exports = function changelogPlugin(context) {
  const {siteDir, baseUrl} = context;

  return {
    name: 'changelog-plugin',

    async loadContent() {
      const changelogs = [];

      // Check if this is a git repo
      let isGitRepo = false;
      try {
        execSync('git rev-parse --git-dir', {cwd: siteDir, stdio: 'pipe'});
        isGitRepo = true;
      } catch (e) {
        console.warn('[Changelog Plugin] Not in a git repository - changelog will be empty');
      }

      // Generate changelog entries from git commits
      if (isGitRepo) {
        try {
          let commits = [];

          // Get commits from git
          // Use null character as delimiter to avoid conflicts with pipe in commit messages
          const gitLog = execSync(
            `git log --pretty=format:"%H%x00%ad%x00%s%x00%b" --date=short -- .`,
            {
              cwd: siteDir,
              encoding: 'utf-8',
              maxBuffer: 10 * 1024 * 1024,
            }
          ).trim();

          if (gitLog) {
            commits = gitLog.split('\n').map(line => {
              const parts = line.split('\x00');
              const [hash, date, subject, body] = parts;
              // Validate that we have at least hash, date, and subject
              if (!hash || !date || !subject) {
                return null;
              }
              return {hash, date, subject, body: body || ''};
            }).filter(commit => commit !== null);
          }

          if (commits.length > 0) {
            // Group commits by month
            const commitsByMonth = commits.reduce((acc, commit) => {
              const monthKey = commit.date.substring(0, 7); // YYYY-MM
              if (!acc[monthKey]) {
                acc[monthKey] = [];
              }
              acc[monthKey].push(commit);
              return acc;
            }, {});

            // Create changelog entries for each month
            Object.entries(commitsByMonth).forEach(([monthKey, monthCommits]) => {
              const [year, month] = monthKey.split('-');
              const monthName = new Date(year, parseInt(month) - 1).toLocaleString('en-US', {
                month: 'long',
                year: 'numeric',
              });

              changelogs.push({
                id: `git-${monthKey}`,
                title: monthName,
                date: `${monthKey}-01`,
                version: '',
                commits: monthCommits,
              });
            });
          }
        } catch (error) {
          console.warn('[Changelog Plugin] Could not load git history:', error.message);
        }
      }

      // Sort by date (newest first)
      return changelogs.sort((a, b) => {
        return new Date(b.date) - new Date(a.date);
      });
    },

    async contentLoaded({content, actions}) {
      const {addRoute, createData} = actions;

      // Create a data file that can be imported
      const changelogData = await createData(
        'changelog.json',
        JSON.stringify(content, null, 2)
      );

      const changelogPath = normalizeUrl([baseUrl, '/changelog']);

      // Add route for changelog page
      addRoute({
        path: changelogPath,
        component: '@site/src/plugins/changelog/ChangelogPage.js',
        modules: {
          changelog: changelogData,
        },
      });
    },
  };
};
