package com.reportgen.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.reportgen.dto.CreateReportRequestDto;
import com.reportgen.dto.ReportResponseDto;
import com.reportgen.dto.TemplateResponseDto;
import com.reportgen.model.ReportOutput;
import com.reportgen.model.ReportRequest;
import com.reportgen.model.ReportStatus;
import com.reportgen.model.ReportTemplate;
import com.reportgen.repository.ReportOutputRepository;
import com.reportgen.repository.ReportRequestRepository;
import com.reportgen.repository.ReportTemplateRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.OffsetDateTime;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

@Service
public class ReportService {

    private static final Logger log = LoggerFactory.getLogger(ReportService.class);

    private final ReportTemplateRepository templateRepository;
    private final ReportRequestRepository requestRepository;
    private final ReportOutputRepository outputRepository;
    private final RenderingClient renderingClient;
    private final ObjectMapper objectMapper;

    public ReportService(ReportTemplateRepository templateRepository,
                         ReportRequestRepository requestRepository,
                         ReportOutputRepository outputRepository,
                         RenderingClient renderingClient,
                         ObjectMapper objectMapper) {
        this.templateRepository = templateRepository;
        this.requestRepository = requestRepository;
        this.outputRepository = outputRepository;
        this.renderingClient = renderingClient;
        this.objectMapper = objectMapper;
    }

    /**
     * List all report templates
     */
    @Transactional(readOnly = true)
    public List<TemplateResponseDto> getAllTemplates() {
        return templateRepository.findAll().stream()
                .map(this::mapToTemplateDto)
                .collect(Collectors.toList());
    }

    /**
     * Get a single template by ID
     */
    @Transactional(readOnly = true)
    public TemplateResponseDto getTemplateById(Long id) {
        ReportTemplate template = templateRepository.findById(id)
                .orElseThrow(() -> new IllegalArgumentException("Report template not found with id: " + id));
        return mapToTemplateDto(template);
    }

    /**
     * Submit a report generation request:
     * 1. Store request with status PENDING
     * 2. Call C++ rendering service
     * 3. Persist resulting binary output
     * 4. Update request status to DONE
     */
    @Transactional
    public ReportResponseDto createReport(CreateReportRequestDto dto) {
        ReportTemplate template = templateRepository.findById(dto.getTemplateId())
                .orElseThrow(() -> new IllegalArgumentException("Template not found with id: " + dto.getTemplateId()));

        String requestedFormat = (dto.getFormat() != null && !dto.getFormat().isBlank())
                ? dto.getFormat().toUpperCase()
                : "PDF";

        // Step 1: Save initial PENDING request
        String paramJsonStr;
        try {
            paramJsonStr = objectMapper.writeValueAsString(dto.getParameters());
        } catch (JsonProcessingException e) {
            paramJsonStr = "{}";
        }

        ReportRequest request = ReportRequest.builder()
                .template(template)
                .parameters(paramJsonStr)
                .status(ReportStatus.PENDING)
                .build();

        request = requestRepository.save(request);
        log.info("Saved report request ID={} in PENDING status", request.getId());

        // Step 2 & 3: Render and persist output
        try {
            JsonNode schemaNode = parseJson(template.getTemplateSchema());
            String title = (schemaNode != null && schemaNode.has("title"))
                    ? schemaNode.get("title").asText()
                    : template.getName();

            // Set to PROCESSING
            request.setStatus(ReportStatus.PROCESSING);
            requestRepository.save(request);

            // Call C++ service
            byte[] fileBytes = renderingClient.renderReport(title, schemaNode, dto.getParameters(), requestedFormat);

            // Persist output
            ReportOutput output = ReportOutput.builder()
                    .request(request)
                    .format(requestedFormat)
                    .fileData(fileBytes)
                    .build();
            outputRepository.save(output);

            // Mark as DONE
            request.setStatus(ReportStatus.DONE);
            request.setCompletedAt(OffsetDateTime.now());
            requestRepository.save(request);
            log.info("Report request ID={} completed successfully (format={})", request.getId(), requestedFormat);

        } catch (Exception e) {
            log.error("Failed processing report request ID={}: {}", request.getId(), e.getMessage());
            request.setStatus(ReportStatus.FAILED);
            request.setCompletedAt(OffsetDateTime.now());
            requestRepository.save(request);
        }

        return mapToReportDto(request);
    }

    /**
     * Get report request status & metadata
     */
    @Transactional(readOnly = true)
    public ReportResponseDto getReportById(Long id) {
        ReportRequest request = requestRepository.findById(id)
                .orElseThrow(() -> new IllegalArgumentException("Report request not found with id: " + id));
        return mapToReportDto(request);
    }

    /**
     * List all reports
     */
    @Transactional(readOnly = true)
    public List<ReportResponseDto> getAllReports() {
        return requestRepository.findAllByOrderByRequestedAtDesc().stream()
                .map(this::mapToReportDto)
                .collect(Collectors.toList());
    }

    /**
     * Fetch binary report output
     */
    @Transactional(readOnly = true)
    public ReportOutput getReportOutput(Long requestId) {
        return outputRepository.findByRequestId(requestId)
                .orElseThrow(() -> new IllegalArgumentException("No report output found for request id: " + requestId));
    }

    // Helper: Map template to DTO
    private TemplateResponseDto mapToTemplateDto(ReportTemplate template) {
        return TemplateResponseDto.builder()
                .id(template.getId())
                .name(template.getName())
                .description(template.getDescription())
                .templateSchema(parseJson(template.getTemplateSchema()))
                .outputFormats(parseFormats(template.getOutputFormats()))
                .createdAt(template.getCreatedAt())
                .updatedAt(template.getUpdatedAt())
                .build();
    }

    // Helper: Map report request to DTO
    private ReportResponseDto mapToReportDto(ReportRequest request) {
        String format = "PDF";
        boolean hasOutput = outputRepository.findByRequestId(request.getId()).isPresent();
        if (hasOutput) {
            format = outputRepository.findByRequestId(request.getId()).get().getFormat();
        }

        return ReportResponseDto.builder()
                .id(request.getId())
                .templateId(request.getTemplate().getId())
                .templateName(request.getTemplate().getName())
                .parameters(parseJson(request.getParameters()))
                .status(request.getStatus())
                .format(format)
                .requestedAt(request.getRequestedAt())
                .completedAt(request.getCompletedAt())
                .hasOutput(hasOutput)
                .downloadUrl("/api/reports/" + request.getId() + "/output")
                .build();
    }

    private JsonNode parseJson(String jsonStr) {
        if (jsonStr == null || jsonStr.isBlank()) {
            return objectMapper.createObjectNode();
        }
        try {
            return objectMapper.readTree(jsonStr);
        } catch (Exception e) {
            log.warn("Failed to parse JSON string: {}", jsonStr);
            return objectMapper.createObjectNode();
        }
    }

    private List<String> parseFormats(String formatsStr) {
        if (formatsStr == null) return List.of("PDF");
        String cleaned = formatsStr.replace("{", "").replace("}", "").replace("(", "").replace(")", "").trim();
        if (cleaned.isEmpty()) return List.of("PDF");
        return Arrays.stream(cleaned.split(","))
                .map(String::trim)
                .filter(s -> !s.isEmpty())
                .collect(Collectors.toList());
    }
}
