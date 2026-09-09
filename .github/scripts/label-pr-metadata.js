'use strict';

const fs = require('fs');

const PROJECTS_PATH = '.github/pr-projects.json';
const PROJECT_LABEL_PREFIX = 'project:';
const PROJECT_LABEL_COLOR = '1d76db';

async function removeLabel(github, params) {
  try {
    await github.rest.issues.removeLabel(params);
  } catch (error) {
    if (error.status !== 404) {
      throw error;
    }
  }
}

module.exports = async function labelPullRequestMetadata({
  github,
  context,
  core,
}) {
  const {owner, repo} = context.repo;
  const issueNumber = context.payload.pull_request.number;
  const {data: pullRequest} = await github.rest.pulls.get({
    owner,
    repo,
    pull_number: issueNumber,
  });
  const validCompanies = [
    'accton',
    'celestica',
    'nexthop',
    'arista',
    'cisco',
    'nvidia',
    'meta',
  ];
  const {npiProjects, nonNpiProjects} = JSON.parse(
    fs.readFileSync(PROJECTS_PATH, 'utf8'),
  );
  const projects = [...npiProjects, ...nonNpiProjects];
  const validProjects = new Set(projects.map(project => project.toLowerCase()));
  const currentLabels = await github.paginate(
    github.rest.issues.listLabelsOnIssue,
    {owner, repo, issue_number: issueNumber, per_page: 100},
  );

  async function removeManagedLabels() {
    const managedLabels = currentLabels.filter(
      ({name}) =>
        validCompanies.includes(name.toLowerCase()) ||
        name.toLowerCase().startsWith(PROJECT_LABEL_PREFIX),
    );
    for (const {name} of managedLabels) {
      await removeLabel(github, {
        owner,
        repo,
        issue_number: issueNumber,
        name,
      });
    }
  }

  const match = pullRequest.title.match(/^\[([^\]]+)\]\s*\[([^\]]+)\]\s+.+/);
  if (!match) {
    await removeManagedLabels();
    core.warning(
      'PR title must use the format "[CompanyName][ProjectName] Your PR title", ' +
        'for example "[Accton][minipack3n] Add transceiver support".',
    );
    return;
  }

  const company = match[1].trim().toLowerCase();
  const project = match[2].trim().toLowerCase();
  if (!validCompanies.includes(company)) {
    await removeManagedLabels();
    core.warning(`Company name "${match[1].trim()}" is not recognized.`);
    return;
  }
  if (!validProjects.has(project)) {
    await removeManagedLabels();
    core.warning(
      `Project name "${match[2].trim()}" is not recognized. ` +
        `Supported projects: ${projects.join(', ')}.`,
    );
    return;
  }

  const projectLabel = `${PROJECT_LABEL_PREFIX}${project}`;
  const repositoryLabels = await github.paginate(
    github.rest.issues.listLabelsForRepo,
    {owner, repo, per_page: 100},
  );
  if (!repositoryLabels.some(({name}) => name.toLowerCase() === projectLabel)) {
    await github.rest.issues.createLabel({
      owner,
      repo,
      name: projectLabel,
      color: PROJECT_LABEL_COLOR,
      description: `FBOSS project: ${project}`,
    });
  }

  const desiredLabels = new Set([company, projectLabel]);
  const managedLabels = currentLabels.filter(
    ({name}) =>
      validCompanies.includes(name.toLowerCase()) ||
      name.toLowerCase().startsWith(PROJECT_LABEL_PREFIX),
  );
  for (const {name} of managedLabels) {
    if (!desiredLabels.has(name.toLowerCase())) {
      await removeLabel(github, {
        owner,
        repo,
        issue_number: issueNumber,
        name,
      });
    }
  }

  await github.rest.issues.addLabels({
    owner,
    repo,
    issue_number: issueNumber,
    labels: [company, projectLabel],
  });
  core.info(`Added labels "${company}" and "${projectLabel}".`);
};
