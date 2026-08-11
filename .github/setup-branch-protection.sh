#!/usr/bin/env bash
#
# Apply branch protection to master and develop so that:
#   * direct pushes are blocked (a pull request is required),
#   * the "Build and test" and "Enforce merge source" checks must pass,
#   * the rules apply to administrators too,
#   * force pushes and deletions are disallowed.
#
# Usage: .github/setup-branch-protection.sh <owner/repo>
# Requires: an authenticated `gh` with admin rights on the repository.
#
# Note: run this only after the CI and Branch policy workflows have run at
# least once, so GitHub recognizes the status-check names below.

set -euo pipefail

REPO="${1:-}"
if [ -z "$REPO" ]; then
    echo "usage: $0 <owner/repo>" >&2
    exit 2
fi

protect() {
    local branch="$1"
    echo "Protecting $REPO@$branch ..."
    gh api -X PUT "repos/$REPO/branches/$branch/protection" \
        -H "Accept: application/vnd.github+json" --input - <<'JSON'
{
  "required_status_checks": {
    "strict": true,
    "contexts": ["Build and test", "Enforce merge source"]
  },
  "enforce_admins": true,
  "required_pull_request_reviews": {
    "required_approving_review_count": 0
  },
  "restrictions": null,
  "allow_force_pushes": false,
  "allow_deletions": false
}
JSON
}

protect master
protect develop

echo "Done."
