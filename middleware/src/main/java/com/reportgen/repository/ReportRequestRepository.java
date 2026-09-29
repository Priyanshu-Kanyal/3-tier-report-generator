package com.reportgen.repository;

import com.reportgen.model.ReportRequest;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface ReportRequestRepository extends JpaRepository<ReportRequest, Long> {
    List<ReportRequest> findAllByOrderByRequestedAtDesc();
}
