import { Component, OnInit, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { ReportService } from './services/report.service';
import { ReportTemplate, ReportRequest, CreateReportPayload } from './models/report.model';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {
  title = 'Report Generator';
  private reportService = inject(ReportService);

  templates: ReportTemplate[] = [];
  selectedTemplate: ReportTemplate | null = null;
  formParameters: Record<string, any> = {};
  selectedFormat: string = 'PDF';

  reports: ReportRequest[] = [];
  activeReport: ReportRequest | null = null;

  isLoadingTemplates = false;
  isSubmitting = false;
  isLoadingReports = false;
  errorMessage = '';
  successMessage = '';

  ngOnInit(): void {
    this.loadTemplates();
    this.loadReports();
  }

  loadTemplates(): void {
    this.isLoadingTemplates = true;
    this.errorMessage = '';
    this.reportService.getTemplates().subscribe({
      next: (data) => {
        this.templates = data;
        this.isLoadingTemplates = false;
        if (data.length > 0 && !this.selectedTemplate) {
          this.selectTemplate(data[0]);
        }
      },
      error: (err) => {
        console.error('Failed to load templates:', err);
        this.errorMessage = 'Could not connect to Spring Boot middleware (http://localhost:8080). Make sure it is running.';
        this.isLoadingTemplates = false;
      }
    });
  }

  loadReports(): void {
    this.isLoadingReports = true;
    this.reportService.getReports().subscribe({
      next: (data) => {
        this.reports = data;
        this.isLoadingReports = false;
      },
      error: (err) => {
        console.warn('Failed to load reports:', err);
        this.isLoadingReports = false;
      }
    });
  }

  selectTemplate(template: ReportTemplate): void {
    this.selectedTemplate = template;
    this.formParameters = {};
    this.activeReport = null;
    this.successMessage = '';
    this.errorMessage = '';

    // Initialize default format
    if (template.outputFormats && template.outputFormats.length > 0) {
      this.selectedFormat = template.outputFormats[0];
    } else {
      this.selectedFormat = 'PDF';
    }

    // Initialize default values based on field schema
    if (template.templateSchema && template.templateSchema.fields) {
      for (const field of template.templateSchema.fields) {
        if (field.name === 'region') this.formParameters[field.name] = 'West';
        else if (field.name === 'start_date') this.formParameters[field.name] = '2026-01-01';
        else if (field.name === 'end_date') this.formParameters[field.name] = '2026-01-31';
        else if (field.name === 'warehouse') this.formParameters[field.name] = 'Central Warehouse';
        else this.formParameters[field.name] = '';
      }
    }
  }

  submitReport(): void {
    if (!this.selectedTemplate) return;

    this.isSubmitting = true;
    this.errorMessage = '';
    this.successMessage = '';
    this.activeReport = null;

    const payload: CreateReportPayload = {
      templateId: this.selectedTemplate.id,
      parameters: { ...this.formParameters },
      format: this.selectedFormat
    };

    this.reportService.submitReport(payload).subscribe({
      next: (report) => {
        this.isSubmitting = false;
        this.activeReport = report;
        this.successMessage = `Report Request #${report.id} completed successfully via C++ rendering engine!`;
        this.loadReports();
      },
      error: (err) => {
        this.isSubmitting = false;
        console.error('Failed to submit report request:', err);
        this.errorMessage = err.error?.message || 'Error executing report generation workflow.';
      }
    });
  }

  downloadActiveReport(): void {
    if (!this.activeReport) return;
    this.reportService.downloadReport(
      this.activeReport.id,
      this.activeReport.format,
      this.activeReport.templateName || 'Report'
    );
  }

  downloadExistingReport(report: ReportRequest): void {
    this.reportService.downloadReport(
      report.id,
      report.format,
      report.templateName || 'Report'
    );
  }

  formatDate(dateStr?: string): string {
    if (!dateStr) return 'N/A';
    try {
      const d = new Date(dateStr);
      return d.toLocaleString();
    } catch {
      return dateStr;
    }
  }

  objectKeys(obj: any): string[] {
    return obj ? Object.keys(obj) : [];
  }
}
