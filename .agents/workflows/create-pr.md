---
description: How to create a granular Pull Request
---

# Pull Request Creation Workflow

This workflow guides the AI or a developer on how to correctly structure, commit, and open a Pull Request for the Kaldmor Canary OT repository.

## Principles
- **One PR = One Logical Change**: Do not mix refactoring, bug fixes, and feature additions in a single PR.
- **Title and Description**: Must follow the project's `.github/PULL_REQUEST_TEMPLATE.md`.

## Steps

1. **Ensure your `main` branch is up to date**
```bash
git checkout main
git pull origin main
```

2. **Create a fresh, descriptive branch**
   Name the branch according to the type of change (e.g., `feature/xyz`, `fix/abc`, `refactor/iomap`).
```bash
git checkout -b feature/my_new_feature
```

3. **Stage only relevant files**
   Review uncommitted changes and add ONLY the files related to this specific logical change.
```bash
git add src/my_changed_file.cpp
```

4. **Commit the changes**
   Write a clear, descriptive commit message.
```bash
git commit -m "Refactor: extracted logic from my_changed_file.cpp"
```

5. **Push to the remote**
```bash
git push origin <branch_name>
```

6. **Create the Pull Request**
   Use GitHub CLI (`gh`) or the web interface. If using `gh`:
```bash
// turbo
gh pr create --title "Descriptive Title" --body-file .github/PULL_REQUEST_TEMPLATE.md
```
   *(Note: Remember to fill out the template before submitting!)*
