#include "test.h"
#if !LELY_NO_CO_CSDO
#include <lely/co/csdo.hpp>
#endif
#if !LELY_NO_CO_DCF
#include <lely/co/dcf.hpp>
#endif
#include <lely/co/dev.hpp>
#if !LELY_NO_CO_EMCY
#include <lely/co/emcy.hpp>
#endif
#if !LELY_NO_CO_GW
#include <lely/co/gw.hpp>
#endif
#if !LELY_NO_CO_GW_TXT
#include <lely/co/gw_txt.hpp>
#endif
#if !LELY_NO_CO_LSS
#include <lely/co/lss.hpp>
#endif
#include <lely/co/nmt.hpp>
#include <lely/co/obj.hpp>
#if !LELY_NO_CO_RPDO
#include <lely/co/rpdo.hpp>
#endif
#if !LELY_NO_CO_SDEV
#include <lely/co/sdev.hpp>
#endif
#include <lely/co/ssdo.hpp>
#if !LELY_NO_CO_SYNC
#include <lely/co/sync.hpp>
#endif
#if !LELY_NO_CO_TIME
#include <lely/co/time.hpp>
#endif
#if !LELY_NO_CO_RPDO
#include <lely/co/tpdo.hpp>
#endif
#include <lely/co/type.hpp>
#include <lely/co/val.hpp>
#if !LELY_NO_CO_WTM
#include <lely/co/wtm.hpp>
#endif

int
main() {
  tap_plan(0);

  return 0;
}
