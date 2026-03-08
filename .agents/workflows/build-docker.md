---
description: How to build and run the Canary server using Docker
---

# Docker Build & Run Workflow

This workflow describes how to build and start the Kaldmor Canary OT server using Docker Compose.

## Prerequisites
- Docker and Docker Compose installed.
- Ensure the `.env` file is generated and configured.
- Ensure `data-otservbr-global/` or `data-canary/` datapacks are correctly placed or configured in `config.lua`.

## Steps

1. **Navigate to the project root**
   Ensure you are in the root directory of the Kaldmor Canary OT repository.

2. **Start the containers**
   Run Docker Compose in detached mode, forcing a build of the images if necessary.
```bash
// turbo
docker compose up -d --build
```

3. **Check the logs**
   Monitor the initialization process of the server to ensure it started without errors.
```bash
docker compose logs -f canary
```

4. **Verify Database connection**
   The database should be initialized and accessible. If `phpMyAdmin` is configured, it will be available on the mapped port (usually 8080).

5. **Stop the containers (Teardown)**
   To stop the server once development/testing is complete:
```bash
docker compose down
```
