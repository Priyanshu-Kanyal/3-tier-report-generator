package com.reportgen.repository;

import com.reportgen.model.ReportOutput;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface ReportOutputRepository extends JpaRepository<ReportOutput, Long> {
    Optional<ReportOutput> findByRequestId(Long requestId);
}
