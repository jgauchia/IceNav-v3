/**
 * @file diag.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Boot diagnostics: reset reason logging and crash coredump recovery
 * @version 0.3.3
 * @date 2026-10
 */

#pragma once

void diagBootReport();
const char *diagSnapshotMemory();
