export interface TemplateField {
  name: string;
  type: string; // 'string' | 'date' | 'number'
  label: string;
  required?: boolean;
}

export interface TemplateSchema {
  title: string;
  fields: TemplateField[];
  columns: string[];
}

export interface ReportTemplate {
  id: number;
  name: string;
  description: string;
  templateSchema: TemplateSchema;
  outputFormats: string[];
  createdAt?: string;
  updatedAt?: string;
}

export type ReportStatus = 'PENDING' | 'PROCESSING' | 'DONE' | 'FAILED';

export interface ReportRequest {
  id: number;
  templateId: number;
  templateName: string;
  parameters: Record<string, any>;
  status: ReportStatus;
  format: string;
  requestedAt: string;
  completedAt?: string;
  hasOutput: boolean;
  downloadUrl: string;
}

export interface CreateReportPayload {
  templateId: number;
  parameters: Record<string, any>;
  format: string;
}
