-- Database Schema for Report Generator Demo Application
-- Compatible with PostgreSQL 13+

DROP TABLE IF EXISTS report_outputs CASCADE;
DROP TABLE IF EXISTS report_requests CASCADE;
DROP TABLE IF EXISTS report_templates CASCADE;

CREATE TABLE report_templates (
    id SERIAL PRIMARY KEY,
    name VARCHAR(150) NOT NULL,
    description TEXT,
    template_schema JSONB NOT NULL,
    output_formats TEXT NOT NULL DEFAULT '{PDF}',
    created_at TIMESTAMPTZ DEFAULT now(),
    updated_at TIMESTAMPTZ DEFAULT now()
);

CREATE TABLE report_requests (
    id SERIAL PRIMARY KEY,
    template_id INT NOT NULL REFERENCES report_templates(id),
    parameters JSONB NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'PENDING', -- PENDING | PROCESSING | DONE | FAILED
    requested_at TIMESTAMPTZ DEFAULT now(),
    completed_at TIMESTAMPTZ
);

CREATE TABLE report_outputs (
    id SERIAL PRIMARY KEY,
    request_id INT NOT NULL REFERENCES report_requests(id),
    format VARCHAR(10) NOT NULL, -- PDF | XLSX | CSV
    file_data BYTEA,
    generated_at TIMESTAMPTZ DEFAULT now()
);

-- Seed Data

-- 1. Sales Summary Template
INSERT INTO report_templates (name, description, template_schema, output_formats) VALUES (
    'Sales Summary',
    'Regional sales report over a date range',
    '{
        "title": "Sales Summary Report",
        "fields": [
            {"name": "region", "type": "string", "label": "Region"},
            {"name": "start_date", "type": "date", "label": "Start Date"},
            {"name": "end_date", "type": "date", "label": "End Date"}
        ],
        "columns": ["date", "product", "quantity", "revenue"]
    }',
    '{PDF,XLSX}'
);

-- 2. Inventory Status Template
INSERT INTO report_templates (name, description, template_schema, output_formats) VALUES (
    'Inventory Status',
    'Current stock levels by warehouse',
    '{
        "title": "Inventory Status Report",
        "fields": [
            {"name": "warehouse", "type": "string", "label": "Warehouse"}
        ],
        "columns": ["sku", "product_name", "quantity_on_hand", "reorder_level"]
    }',
    '{PDF}'
);

-- 3. Initial Demo Report Request (Sales Summary)
INSERT INTO report_requests (template_id, parameters, status, requested_at, completed_at) VALUES (
    1,
    '{"region": "West", "start_date": "2026-01-01", "end_date": "2026-01-31"}',
    'DONE',
    now(),
    now()
);

-- 4. Initial Demo Report Output for Request #1
-- Sample PDF binary placeholder data for seed demo
INSERT INTO report_outputs (request_id, format, file_data, generated_at) VALUES (
    1,
    'PDF',
    decode('255044462d312e340a25e2e3cfd30a312030206f626a0a3c3c202f54797065202f436174616c6f67202f5061676573203220302052203e3e0a656e646f626a0a322030206f626a0a3c3c202f54797065202f5061676573202f4b696473205b33203020525d202f436f756e742031203e3e0a656e646f626a0a', 'hex'),
    now()
);
