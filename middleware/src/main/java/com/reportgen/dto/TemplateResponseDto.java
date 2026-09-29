package com.reportgen.dto;

import com.fasterxml.jackson.databind.JsonNode;
import lombok.*;

import java.time.OffsetDateTime;
import java.util.List;

@Getter
@Setter
@NoArgsConstructor
@AllArgsConstructor
@Builder
public class TemplateResponseDto {
    private Long id;
    private String name;
    private String description;
    private JsonNode templateSchema;
    private List<String> outputFormats;
    private OffsetDateTime createdAt;
    private OffsetDateTime updatedAt;
}
