package com.reportgen.dto;

import com.fasterxml.jackson.databind.JsonNode;
import jakarta.validation.constraints.NotNull;
import lombok.*;

@Getter
@Setter
@NoArgsConstructor
@AllArgsConstructor
@Builder
public class CreateReportRequestDto {

    @NotNull(message = "Template ID is required")
    private Long templateId;

    @NotNull(message = "Parameters are required")
    private JsonNode parameters;

    @Builder.Default
    private String format = "PDF";
}
