'use strict';

const fs = require('fs');

const CONFIG_PATH = '.github/pr-owner-routing.json';

// Randomly select one from list of eligible owners
function selectOwner(owners) {
  if (owners.length === 0) {
    return '';
  }
  return owners[Math.floor(Math.random() * owners.length)];
}

/*
Match based on the most-affected service, counted by number
of lines changed under each service's paths
*/
function findServiceForFiles(files, services) {
  if (files.length === 0 || !Array.isArray(services)) {
    return undefined;
  }

  const linesByService = new Map();
  for (const file of files) {
    const matchingServices = services.filter(
      (service) =>
        Array.isArray(service.paths) &&
        service.paths.some((prefix) =>
          file.filename.startsWith(prefix)
        ),
    );

    // Skip paths that are unknown or ambiguously configured.
    if (matchingServices.length !== 1) {
      continue;
    }

    const service = matchingServices[0];
    const linesTouched = file.changes;
    const currentTotal = linesByService.get(service.name)?.linesTouched || 0;
    linesByService.set(service.name, {
      service,
      linesTouched: currentTotal + linesTouched,
    });
  }

  const rankedServices = [...linesByService.values()].sort(
    (first, second) => second.linesTouched - first.linesTouched,
  );
  return rankedServices[0]?.service;
}

/*
PR assignment algorithm:
1. If already has assignee (set by PR author), no-op
2. If PR touches service files, use the list of service owners
   based on the service which has the most lines affected
3. If no service files touched (likely just configs) assign
   to vendor owners
4. If neither, use fallback owners
5. Randomly select one from list of eligible owners
*/
module.exports = async function assignPullRequestOwner({
  github,
  context,
  core,
}) {
  const { owner, repo } = context.repo;
  const pullNumber = context.issue.number;
  const pullRequest = context.payload.pull_request;

  if (pullRequest.assignees.length > 0) {
    core.info(
      `PR already has an owner: ${pullRequest.assignees
        .map(({ login }) => login)
        .join(', ')}`,
    );
    return;
  }

  // Both this script and its configuration come from the trusted base commit
  // checked out by the workflow, never from the contributor's branch.
  const config = JSON.parse(fs.readFileSync(CONFIG_PATH, 'utf8'));
  const changedFiles = await github.paginate(github.rest.pulls.listFiles, {
    owner,
    repo,
    pull_number: pullNumber,
    per_page: 100,
  });

  const service = findServiceForFiles(changedFiles, config.services);
  const vendorMatch = pullRequest.title.match(/^\[([^\]]+)\]/);
  const vendor = vendorMatch?.[1].trim().toLowerCase();
  const serviceOwners = service?.owners || [];
  const vendorOwners = vendor ? config.ownersByVendor[vendor] || [] : [];

  let routingMessage = `Routing to fallback owner `;
  let eligibleOwners = config.fallbackOwners;
  if (serviceOwners.length > 0) {
    eligibleOwners = serviceOwners;
    routingMessage = `Routing service "${service.name}" to `;
  } else if (vendorOwners.length > 0) {
    routingMessage = `Routing vendor "${vendor}" to `;
    eligibleOwners = vendorOwners;
  }
  const selectedOwner = selectOwner(eligibleOwners);
  if (!selectedOwner) {
    core.warning('No owner is configured for this PR.');
    return;
  }
  core.info(routingMessage + selectedOwner);

  await github.rest.issues.addAssignees({
    owner,
    repo,
    issue_number: pullNumber,
    assignees: [selectedOwner],
  });

  // GitHub may silently ignore an assignee who cannot be assigned to the
  // repository, so verify that the update actually took effect.
  const { data: updatedPullRequest } = await github.rest.pulls.get({
    owner,
    repo,
    pull_number: pullNumber,
  });

  const assignedLogins = updatedPullRequest.assignees.map(({ login }) =>
    login.toLowerCase(),
  );
  if (!assignedLogins.includes(selectedOwner.toLowerCase())) {
    core.setFailed(
      `GitHub did not assign @${selectedOwner}. Confirm that the user has access to ${owner}/${repo}.`,
    );
    return;
  }

  if (updatedPullRequest.assignees.length !== 1) {
    core.setFailed(
      `PR has ${updatedPullRequest.assignees.length} assignees; the ownership policy expects exactly one.`,
    );
  }

  core.info(`Assigned @${selectedOwner} as the PR owner.`);
};
