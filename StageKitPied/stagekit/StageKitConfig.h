#ifndef _STAGEKITCONFIG_H_
#define _STAGEKITCONFIG_H_

struct StageKitConfig {
  bool m_light_pod_enabled;
  bool m_strobe_enabled;
  bool m_fog_enabled;
  long m_fog_instance_time_max_ms;
  long m_fog_total_time_max_ms;
};

#endif
