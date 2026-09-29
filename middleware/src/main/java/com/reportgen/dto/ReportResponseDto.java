package com.reportgen.dto;

import com.fasterxml.jackson.databind.JsonNode;
import com.reportgen.model.ReportStatus;
import lombok.*;

import java.time.OffsetDateTime;

@Getter
@Setter
@NoArgsConstructor
@AllArgsConstructor
@Builder
public class ReportResponseDto {
    private Long id;
    private Long templateId;
    private String templateName;
    private JsonNode parameters;
    private ReportStatus status;
    private String format;
    private OffsetDateTime requestedAt;
    private OffsetDateTime completedAt;
    private boolean hasOutput;
    private String downloadUrl;
}
