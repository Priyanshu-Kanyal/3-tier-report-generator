import { Injectable, inject } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';
import { ReportTemplate, ReportRequest, CreateReportPayload } from '../models/report.model';

@Injectable({
  providedIn: 'root'
})
export class ReportService {
  private http = inject(HttpClient);
  private baseUrl = 'http://localhost:8080/api';

  getTemplates(): Observable<ReportTemplate[]> {
    return this.http.get<ReportTemplate[]>(`${this.baseUrl}/templates`);
  }

  getTemplate(id: number): Observable<ReportTemplate> {
    return this.http.get<ReportTemplate>(`${this.baseUrl}/templates/${id}`);
  }

  getReports(): Observable<ReportRequest[]> {
    return this.http.get<ReportRequest[]>(`${this.baseUrl}/reports`);
  }

  getReport(id: number): Observable<ReportRequest> {
    return this.http.get<ReportRequest>(`${this.baseUrl}/reports/${id}`);
  }

  submitReport(payload: CreateReportPayload): Observable<ReportRequest> {
    return this.http.post<ReportRequest>(`${this.baseUrl}/reports`, payload);
  }

  downloadReport(requestId: number, format: string, templateName: string): void {
    const url = `${this.baseUrl}/reports/${requestId}/output`;
    this.http.get(url, { responseType: 'blob' }).subscribe({
      next: (blob) => {
        const ext = format.toLowerCase() === 'xlsx' ? 'xlsx' : (format.toLowerCase() === 'csv' ? 'csv' : 'pdf');
        const filename = `${templateName.toLowerCase().replace(/\s+/g, '_')}_${requestId}.${ext}`;
        const objectUrl = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = objectUrl;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        window.URL.revokeObjectURL(objectUrl);
      },
      error: (err) => {
        console.error('Error downloading report output:', err);
        alert('Failed to download report output file.');
      }
    });
  }
}
