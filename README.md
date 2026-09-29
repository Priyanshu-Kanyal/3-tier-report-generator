# 3-Tier "Report Generator" Application

A full-stack, enterprise-style demo application demonstrating high-performance report generation across three tiers:
1. **Frontend:** Angular 18 (with modern standalone components, dynamic JSON-schema parameter forms, real-time polling, and direct PDF/XLSX download views).
2. **Middleware:** Java 17 / Spring Boot 3 REST service (manages PostgreSQL persistence, orchestrates report requests, coordinates with C++ renderer).
3. **Backend Engine:** C++ Native REST service (exposing `POST /render`, generating real PDF and Excel/CSV binaries with microsecond-level performance).
4. **Database:** PostgreSQL with full relational schema, JSONB schema definitions, and seed data.

> **Renovate Upgrade Practice**: All three tiers are intentionally configured with slightly older, stable dependency versions (Angular 18.0, Spring Boot 3.1.2, older Jackson, Lombok, PostgreSQL driver, and C++ vcpkg/conan manifests) and a pre-configured `renovate.json` so you can use this project to practice automated dependency management and upgrades with **Renovate Bot**.

---

## Architecture & Communication Flow

```
┌────────────────────────────────┐
│   Angular 18 Frontend          │
│   http://localhost:4200        │
└───────────────┬────────────────┘
                │ HTTP REST (JSON)
                ▼
┌────────────────────────────────┐       JDBC       ┌────────────────────────────────┐
│   Spring Boot 3 (Java 17)      │ ◄──────────────► │   PostgreSQL 15+               │
│   http://localhost:8080        │                  │   localhost:5432/reports_db    │
└───────────────┬────────────────┘                  └────────────────────────────────┘
                │ HTTP REST (JSON payload)
                │ Binary file stream return (PDF / XLSX)
                ▼
┌────────────────────────────────┐
│   C++ Native Rendering Engine  │
│   http://localhost:8081        │
└────────────────────────────────┘
```

### End-to-End Execution Flow
1. User opens the Angular frontend at `http://localhost:4200`.
2. Angular queries `GET /api/templates` from Spring Boot (`:8080`).
3. User selects a template (e.g., **"Sales Summary"**).
4. Angular dynamically inspects `template_schema.fields` and renders matching form inputs:
   - `region` (string)
   - `start_date` (date)
   - `end_date` (date)
   - Output format selection: `PDF` or `XLSX` (from `output_formats`)
5. User clicks **"Generate Report"**:
   - Angular sends `POST /api/reports` with `{ "templateId": 1, "parameters": { "region": "West", "start_date": "2026-01-01", "end_date": "2026-01-31" }, "format": "PDF" }`.
   - Spring Boot persists a row in `report_requests` with status `PENDING`.
   - Spring Boot issues `POST http://localhost:8081/render` to the C++ service with the template schema and parameters.
   - C++ service renders the binary PDF or Excel document.
   - Spring Boot stores the binary bytes in `report_outputs` (`request_id`, `format`, `file_data`, `generated_at`).
   - Spring Boot marks the request status as `DONE`.
6. Angular UI displays the **"Download PDF"** button, triggering `GET /api/reports/{id}/output` to download the file directly in the browser.

---

## Directory Structure

```text
├── db/
│   └── schema.sql                # PostgreSQL table definitions and seed data
├── backend-cpp/
│   ├── src/
│   │   └── main.cpp              # C++ HTTP REST microservice and PDF/Excel generator
│   ├── CMakeLists.txt            # CMake configuration
│   ├── Makefile                  # GCC Makefile
│   ├── vcpkg.json                # vcpkg package manifest (for Renovate practice)
│   ├── conanfile.txt             # Conan package manifest (for Renovate practice)
│   └── render_service.exe        # Pre-compiled Windows x64 binary
├── middleware/
│   ├── pom.xml                   # Maven build with Spring Boot 3.1.2 & dependencies
│   └── src/main/
│       ├── java/com/reportgen/   # Spring Boot controllers, entities, repositories, services
│       └── resources/            # application.properties and application-local.properties
├── frontend/
│   ├── package.json              # Angular 18 dependencies (for Renovate practice)
│   ├── angular.json              # Angular CLI project configuration
│   └── src/                      # TypeScript components, dynamic schema forms, and styles
├── docker-compose.yml            # 1-command PostgreSQL setup
├── renovate.json                 # Renovate configuration with package grouping rules
├── run-all.bat                   # Master launcher script for all 3 tiers
├── run-cpp.bat                   # Launcher for C++ service
├── run-middleware.bat            # Launcher for Spring Boot middleware
└── run-frontend.bat              # Launcher for Angular frontend
```

---

## Prerequisites & Pre-Installed Toolchain

All necessary compilers, runtimes, and tools are installed with full user permissions in `C:\Users\kanya\.tools`:
- **Node.js**: v24.19.0 (Global)
- **Java JDK**: Eclipse Adoptium OpenJDK 17 (`C:\Users\kanya\.tools\jdk-17`)
- **Maven**: Apache Maven 3.9.6 (`C:\Users\kanya\.tools\maven`)
- **C++ Compiler**: GCC / G++ 16.2.0 (`C:\Users\kanya\.tools\w64devkit`)

---

## Quick Start (Run All Tiers)

To start the entire application with one command:
```cmd
run-all.bat
```
This automatically opens three console windows starting:
1. C++ Native Rendering Engine on `http://localhost:8081`
2. Spring Boot Middleware on `http://localhost:8080` (with Swagger at `http://localhost:8080/swagger-ui.html`)
3. Angular Frontend on `http://localhost:4200`

---

## Running Each Service Individually

### 1. Database Setup (PostgreSQL)

#### Option A: Using Docker Compose (Recommended)
```bash
docker compose up -d
```
This automatically mounts `db/schema.sql` and initializes the `reports_db` database on port 5432.

#### Option B: Using Local PostgreSQL CLI (`psql`)
If PostgreSQL is installed locally:
```bash
createdb -U postgres reports_db
psql -U postgres -d reports_db -f db/schema.sql
```

#### Option C: In-Memory Mode (Zero Database Dependency)
If you don't have PostgreSQL running yet, Spring Boot includes an automated in-memory PostgreSQL-compatible H2 mode:
```cmd
run-middleware.bat
```
*(Runs with `spring.profiles.active=local`, seeding all templates and outputs in memory automatically).*

---

### 2. C++ Backend Rendering Service

Port: **8081** | Endpoint: `POST /render`

#### Run:
```cmd
run-cpp.bat
```
Or manually:
```cmd
cd backend-cpp
render_service.exe 8081
```

#### Recompile (if modifying C++ source):
```cmd
cd backend-cpp
g++ -std=c++17 -O2 src/main.cpp -o render_service.exe -lws2_32
```

#### Test via cURL:
```bash
# Health check:
curl http://localhost:8081/health

# Render PDF:
curl -X POST http://localhost:8081/render \
  -H "Content-Type: application/json" \
  -d "{\"format\":\"PDF\",\"title\":\"Sales Summary Report\",\"parameters\":{\"region\":\"West\",\"start_date\":\"2026-01-01\",\"end_date\":\"2026-01-31\"},\"columns\":[\"date\",\"product\",\"quantity\",\"revenue\"]}" \
  --output test_report.pdf
```

---

### 3. Java 17 / Spring Boot Middleware

Port: **8080** | Base URL: `http://localhost:8080`

#### Run:
```cmd
run-middleware.bat
```
Or manually:
```cmd
cd middleware
mvn spring-boot:run
```

#### Endpoints:
- `GET /api/templates`: List all report templates
- `GET /api/templates/{id}`: Inspect single template schema
- `POST /api/reports`: Submit report request, invokes C++ engine, stores output
- `GET /api/reports`: List recent reports with execution status
- `GET /api/reports/{id}`: Check report request status (`PENDING`, `PROCESSING`, `DONE`, `FAILED`)
- `GET /api/reports/{id}/output`: Stream/download generated binary PDF or Excel file
- `GET /swagger-ui.html`: Interactive OpenAPI / Swagger documentation

---

### 4. Angular 18 Frontend

Port: **4200** | URL: `http://localhost:4200`

#### Run:
```cmd
run-frontend.bat
```
Or manually:
```cmd
cd frontend
npm start
```
Open **http://localhost:4200** in your browser.

---

## Renovate Upgrade Practice Guide

This project is configured specifically for practicing automated dependency management with **Renovate**:

### Configured Package Managers & Files
1. **Frontend (`frontend/package.json`)**:
   - Angular core packages: `^18.0.0`
   - RxJS: `~7.8.0`
   - Zone.js: `~0.14.3`
   - TypeScript: `~5.4.2`
   - Dev dependencies: `@angular/cli@^18.0.0`, `@angular-devkit/build-angular@^18.0.0`
2. **Middleware (`middleware/pom.xml`)**:
   - `spring-boot-starter-parent`: `3.1.2`
   - `postgresql`: `42.6.0`
   - `springdoc-openapi-starter-webmvc-ui`: `2.2.0`
   - `lombok`: `1.18.28`
   - `commons-io`: `2.11.0`
3. **C++ Backend (`backend-cpp/vcpkg.json` and `backend-cpp/conanfile.txt`)**:
   - `nlohmann-json`: `3.10.0`
   - `cpp-httplib`: `0.12.0`
   - `libharu`: `2.3.0`
   - `libxlsxwriter`: `1.1.4`

### How to Run Renovate Locally or in CI
1. Push this repository to GitHub or GitLab.
2. Install the **Renovate GitHub App** on the repository, or run the Renovate CLI locally:
   ```bash
   npx renovate --dry-run=full --token="<your-github-token>" <your-org>/<your-repo>
   ```
3. Renovate will automatically inspect `renovate.json`, create the Onboarding PR, and subsequently generate individual or grouped PRs:
   - Group `angular monorepo` updates
   - Group `spring-boot dependencies` updates
   - Group `cpp libraries` updates
4. You can practice reviewing, testing, resolving breaking changes, and merging automated dependency upgrades!
