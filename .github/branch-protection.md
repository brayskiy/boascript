# Branch protection

The workflows in `.github/workflows/` provide two required status checks:

* **Build and test** (`ci.yml`) — builds the library and runs every test
  suite. Satisfies "merge only after all tests pass".
* **Enforce merge source** (`branch-policy.yml`) — fails a pull request whose
  source branch is not allowed for its target.

A workflow **cannot** block a direct `git push`, so the "direct commits are
prohibited" rules are enforced by GitHub **branch protection**. Apply the
rules below to `master` and `develop` (run once, after the two workflows have
run at least once so GitHub knows the check names).

## Rules

For both `master` and `develop`:

* Require a pull request before merging (blocks direct pushes).
* Require the **Build and test** and **Enforce merge source** status checks
  to pass, and require the branch to be up to date first.
* Include administrators (so the rule is strict for everyone).
* Disallow force pushes and deletions.

The source-branch restriction itself (master ← only `develop`; develop ←
only work branches) is enforced by the **Enforce merge source** check.

## Apply with the GitHub CLI

Run [`setup-branch-protection.sh`](setup-branch-protection.sh):

```sh
.github/setup-branch-protection.sh brayskiy/boascript
```

It requires admin on the repository and an authenticated `gh`.
