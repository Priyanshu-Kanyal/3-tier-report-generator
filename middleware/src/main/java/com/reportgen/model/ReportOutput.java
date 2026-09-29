package com.reportgen.model;

import jakarta.persistence.*;
import lombok.*;
import org.hibernate.annotations.CreationTimestamp;

import java.time.OffsetDateTime;

@Entity
@Table(name = "report_outputs")
@Getter
@Setter
@NoArgsConstructor
@AllArgsConstructor
@Builder
public class ReportOutput {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @OneToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "request_id", nullable = false)
    private ReportRequest request;

    @Column(nullable = false, length = 10)
    private String format;

    @Column(name = "file_data", length = 50000000)
    private byte[] fileData;

    @CreationTimestamp
    @Column(name = "generated_at", updatable = false)
    private OffsetDateTime generatedAt;
}
