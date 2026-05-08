# XIT1299 License Key Manager

A license key management system with web dashboard, REST API, and DLL integration support.

## Run & Operate

- `pnpm --filter @workspace/api-server run dev` — run the API server (port 5000)
- `pnpm run typecheck` — full typecheck across all packages
- `pnpm run build` — typecheck + build all packages
- `pnpm --filter @workspace/api-spec run codegen` — regenerate API hooks and Zod schemas from the OpenAPI spec
- `pnpm --filter @workspace/db run push` — push DB schema changes (dev only)
- Required env: `DATABASE_URL` — Postgres connection string, `SESSION_SECRET` — JWT signing secret

## Stack

- pnpm workspaces, Node.js 24, TypeScript 5.9
- API: Express 5
- DB: PostgreSQL + Drizzle ORM
- Auth: JWT (jsonwebtoken + bcryptjs)
- Validation: Zod (`zod/v4`), `drizzle-zod`
- API codegen: Orval (from OpenAPI spec)
- Build: esbuild (CJS bundle)
- Frontend: React + Vite + Tailwind CSS + shadcn/ui

## Where things live

- `lib/api-spec/openapi.yaml` — OpenAPI spec (source of truth for all API contracts)
- `lib/db/src/schema/` — Drizzle DB schema (users, license_keys tables)
- `artifacts/api-server/src/routes/` — Express route handlers (auth, keys, health)
- `artifacts/api-server/src/middlewares/auth.ts` — JWT middleware
- `artifacts/web/src/` — React frontend

## Architecture decisions

- JWT tokens stored in localStorage, passed as Bearer header to all API calls
- License keys generated in format `XIT-XXXXXXXX-XXXXXXXX-XXXXXXXX` (hex)
- Key durations: 1h, 1d, 1w, 1m (30 days)
- `/api/keys/validate` endpoint is public (no auth required) — designed for DLL use
- `revoked` keys remain in DB; `deleted` keys are removed permanently

## Product

- **Web Dashboard** — login + license key management (generate, list, revoke, delete)
- **REST API** — full CRUD for keys, auth, and public validation endpoint
- **DLL Integration** — C/C++ library can call `POST /api/keys/validate` to verify keys

## User preferences

- Branding: @XIT1299 / @xit1299
- Language: Albanian (shqip)
- Dark theme, command-center aesthetic

## Default credentials

- Username: `admin`
- Password: `admin123`

## Gotchas

- Run codegen after every OpenAPI spec change: `pnpm --filter @workspace/api-spec run codegen`
- The codegen script auto-fixes `lib/api-zod/src/index.ts` after orval runs (echo command in package.json)
- `lib/api-client-react/package.json` exports both `.` and `./custom-fetch` subpaths
