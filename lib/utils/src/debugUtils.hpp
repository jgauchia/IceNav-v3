#if CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG

#include "esp_core_dump.h"

void inline checkCoreDumpPartition() {
  esp_core_dump_init();
  esp_core_dump_summary_t *summary =
      static_cast<esp_core_dump_summary_t *>(malloc(sizeof(esp_core_dump_summary_t)));
  if (summary) {
    esp_err_t err = esp_core_dump_get_summary(summary);
    if (err == ESP_OK) {
      log_i("Getting core dump summary ok.");

    } else {
      log_e("Getting core dump summary not ok. Error: %d", (int)err);
      log_e("Probably no coredump present yet.");
      log_e("esp_core_dump_image_check() = %d", esp_core_dump_image_check());
    }
    free(summary);
  }
}

#else
void inline checkCoreDumpPartition() {}
#endif