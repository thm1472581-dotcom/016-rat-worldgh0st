#if !defined(AFX_HOSTCONFIG_H_INCLUDED)
#define AFX_HOSTCONFIG_H_INCLUDED

// Unique marker embedded in Server binary; Client searches & patches host over it.
// Slot is large enough for IPv4 or DNS names (DDNS).
// SEARCH pattern includes trailing underscores so it only matches g_szCfgHost,
// not a short string-literal copy of the marker prefix.
#define WG_HOST_MARKER        "WGHOST_CFG_HOST_V1"
#define WG_HOST_MARKER_LEN    17
#define WG_HOST_SEARCH        "WGHOST_CFG_HOST_V1____"
#define WG_HOST_SEARCH_LEN    21
#define WG_HOST_SLOT_SIZE     256

#endif
