cmake_minimum_required(VERSION 3.4.1)

set( SOURCES
  # Common V8 objects for JNI/JSC
  Common/ContextGroup.cpp
  Common/JSContext.cpp
  Common/JSValue.cpp
  Common/LoopPreserver.cpp

  # JNI API
  JNI/JNI_JSContext.cpp
  JNI/JNI_JSContextGroup.cpp
  JNI/JNI_JSObject.cpp
  JNI/JNI_JSValue.cpp
  JNI/JNI_LoopPreserver.cpp
  JNI/JNI_OnLoad.cpp
  JNI/JNIJSException.cpp
  JNI/JSFunction.cpp
  JNI/SharedWrap.cpp

  # JavaScriptCore -> V8 bridge
  JSC/JSC_JSBase.cpp
  JSC/JSC_JSContext.cpp
  JSC/JSC_JSContextGroup.cpp
  JSC/JSC_JSObject.cpp
  JSC/JSC_JSString.cpp
  JSC/JSC_JSValue.cpp
  JSC/ObjectData.cpp
  JSC/OpaqueJSClass.cpp
  JSC/OpaqueJSContext.cpp
  JSC/OpaqueJSContextGroup.cpp
  JSC/OpaqueJSString.cpp
  JSC/OpaqueJSValue.cpp

  # libuv
  ../../../../deps/node-10.15.3/deps/uv/src/fs-poll.c
  ../../../../deps/node-10.15.3/deps/uv/src/inet.c
  ../../../../deps/node-10.15.3/deps/uv/src/threadpool.c
  ../../../../deps/node-10.15.3/deps/uv/src/timer.c
  ../../../../deps/node-10.15.3/deps/uv/src/uv-data-getter-setters.c
  ../../../../deps/node-10.15.3/deps/uv/src/uv-common.c
  ../../../../deps/node-10.15.3/deps/uv/src/version.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/async.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/core.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/dl.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/fs.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/getaddrinfo.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/getnameinfo.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/loop.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/loop-watcher.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/pipe.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/poll.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/process.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/signal.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/stream.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/tcp.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/thread.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/tty.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/udp.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/proctitle.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/linux-core.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/linux-inotify.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/linux-syscalls.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/pthread-fixes.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/android-ifaddrs.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/procfs-exepath.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/sysinfo-loadavg.c
  ../../../../deps/node-10.15.3/deps/uv/src/unix/sysinfo-memory.c
        
  # icui18n 
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/nultrans.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/uspoof_conf.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/japancal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/tmunit.cpp
  ../../../../deps/node-10.15.3/tools/icu/patches/62/source/i18n/decimfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/taiwncal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/transreg.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_rounding.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationweights.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/smpdtfst.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationruleparser.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/decNumber.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/fmtable_cnv.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/regeximp.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collation.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_patternmodifier.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationsets.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_decimalquantity.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_asformat.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/usearch.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/ucoleitr.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/double-conversion-fast-dtoa.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_decimfmtprops.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_scientific.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/measure.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/persncal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/ztrans.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/dtfmtsym.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/ucsdet.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationdatawriter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/regextxt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/coleitr.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/csrutf8.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/uitercollationiterator.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/tznames.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationfcd.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/tzrule.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/gender.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_formatimpl.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/msgfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/scientificnumberformatter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/toupptrn.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_stringbuilder.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/sortkey.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/alphaindex.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationkeys.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_affixutils.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationfastlatin.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/choicfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/gregocal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/unumsys.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/sharedbreakiterator.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/numparse_parsednumber.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/format.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/double-conversion.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/scriptset.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/utf16collationiterator.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/uspoof_build.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/casetrn.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/nfrs.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/csdetect.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/numparse_symbols.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/umsg.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/gregoimp.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/dtitvfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationsettings.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/regexst.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_fluent.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/translit.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/plurfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/dtitvinf.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/upluralrules.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationdatareader.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/dtptngen.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_padding.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/fmtable.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/chnsecal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/uregex.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/dtrule.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/ulocdata.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/tridpars.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/cpdtrans.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationbuilder.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_grouping.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/standardplural.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/ethpccal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/olsontz.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/rbt_rule.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationtailoring.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationfastlatinbuilder.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/csrucode.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/astro.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/numfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/rbt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/udatpg.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/numparse_validators.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/strrepl.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/ucal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/double-conversion-cached-powers.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/timezone.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/quant.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/bocsu.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/currpinf.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_notation.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/windtfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/funcrepl.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/calendar.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_multiplier.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/double-conversion-diy-fp.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/smpdtfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/uregion.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_compact.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/rulebasedcollator.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/name2uni.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_longnames.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/stsearch.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/strmatch.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/ucln_in.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/zrule.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/coll.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/quantityformatter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/selfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationrootelements.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/double-conversion-strtod.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/numparse_decimal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/nounit.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/cecal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/titletrn.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/nfsubs.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/numparse_affixes.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/vzone.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/numsys.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/dcfmtsym.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_currencysymbols.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationiterator.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/fphdlimp.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/hebrwcal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/csrsbcs.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/rbt_pars.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/csr2022.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/search.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/zonemeta.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/reldtfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/inputext.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/double-conversion-bignum-dtoa.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/ufieldpositer.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/winnmfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/utf8collationiterator.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/anytrans.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationdata.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/unesctrn.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/ucol_sit.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/remtrans.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/nortrans.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/islamcal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/numparse_scientific.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/dangical.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/currfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/tmutfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_skeletons.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/ucol.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/currunit.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/uspoof.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationroot.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_integerwidth.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/decContext.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_utils.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/double-conversion-bignum.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/rbt_set.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/numparse_stringsegment.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_capi.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/tzfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/uregexc.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/measfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/brktrans.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/tmutamt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/coptccal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/wintzimpl.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/indiancal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_mapper.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/basictz.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/region.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/dayperiodrules.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/csrmbcs.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_modifiers.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/rbnf.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/esctrn.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/ucol_res.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/regexcmp.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/plurrule.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/csmatch.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/simpletz.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/compactdecimalformat.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/tznames_impl.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/numparse_currency.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/fpositer.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/nfrule.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/numparse_compositions.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/utmscale.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/utrans.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/curramt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/measunit.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/buddhcal.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/udateintervalformat.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/uspoof_impl.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/tzgnames.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/rematch.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/datefmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/reldatefmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/vtzone.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/tztrans.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationdatabuilder.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/repattrn.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/collationcompare.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/rbt_data.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/csrecog.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/number_patternstring.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/udat.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/tolowtrn.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/numparse_impl.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/unum.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/rbtz.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n/uni2name.cpp
		
  # icuucx 
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uniset.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/dictbe.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/chariter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/utf_impl.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uhash.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ushape.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnvsel.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uarrsort.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_set.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uniset_props.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/caniter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ubidiwrt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnvscsu.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/cstr.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/wintz.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ubiditransform.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/dictionarydata.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/stringtriebuilder.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/util.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/cstring.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uinit.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucharstrieiterator.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unisetspan.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/icuplug.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_ext.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/servnotf.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/locid.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ustr_cnv.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unames.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/filterednormalizer2.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/loclikely.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uscript_props.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/bytestrieiterator.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/propsvec.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_u32.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/punycode.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ubidiln.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/putil.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/bytestrie.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/resource.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uvector.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucase.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/rbbi.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/locutil.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/servls.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnvdisp.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_err.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unistr_titlecase_brkiter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uloc_tag.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/icudataver.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uenum.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/rbbisetb.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/filteredbrk.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ubrk.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_bld.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/brkeng.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/charstr.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uloc_keytype.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_ct.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/bytestriebuilder.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ubidi.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_u8.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/locdispnames.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_cb.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ustr_titlecase_brkiter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/static_unicode_sets.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/bmpset.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uniset_closure.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/locdspnm.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/bytesinkutil.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/rbbi_cache.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucharstriebuilder.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/utrie2.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unifiedcache.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/servlkf.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/schriter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/locbased.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/udatamem.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/patternprops.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unistr_props.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/stringpiece.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ulist.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ustrtrns.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uiter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/cmemory.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/resbund_cnv.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uvectr64.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ulistformatter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unistr_case.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ustr_wcs.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnvlat1.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/simpleformatter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uts46.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnvmbcs.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/locresdata.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/pluralmap.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uprops.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/usetiter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/propname.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/rbbistbl.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucat.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unifilt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uresdata.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/umapfile.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/messagepattern.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/locavailable.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ustrcase.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/cwchar.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/rbbiscan.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/umath.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uset.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucln_cmn.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ures_cnv.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/normlzr.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucmndata.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/servlk.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/edits.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnvbocu.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ustrfmt.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/parsepos.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/loadednormalizer2impl.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/udata.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/errorcode.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uchriter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_lmb.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucurr.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/udataswp.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ustrcase_locale.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unistr.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnvhz.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/normalizer2.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/sharedobject.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/resbund.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uhash_us.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/serv.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/utrie.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/umutex.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unorm.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uidna.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/servrbf.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_u7.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ruleiter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uset_props.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/appendable.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/utypes.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucasemap.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/usc_impl.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uresbund.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/rbbidata.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unormcmp.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ubidi_props.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uchar.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/normalizer2impl.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnvisci.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unistr_cnv.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/rbbirb.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unifunct.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ustack.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uinvchar.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucasemap_titlecase_brkiter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/bytestream.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ustring.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/usprep.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/listformatter.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/servslkf.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/rbbitblb.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_u16.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/util_props.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uloc.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ustrenum.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_io.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/unistr_case_locale.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uobject.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv_cnv.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucnv2022.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucol_swp.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/dtintrv.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/rbbinode.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/ucharstrie.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uscript.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/utext.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/locmap.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/utrie2_builder.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/uvectr32.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/utrace.cpp
  ../../../../deps/node-10.15.3/deps/icu-small/source/common/brkiter.cpp

  # icustubdata
  ../../../../deps/node-10.15.3/deps/icu-small/source/stubdata/stubdata.cpp
		
)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
  set(SOURCES ${SOURCES}
    ../../androidTest/cpp/testapi.cpp
    ../../androidTest/cpp/CustomGlobalObjectClassTest.cpp
    ../../androidTest/cpp/JSNode.cpp
    ../../androidTest/cpp/JSNodeList.cpp
    ../../androidTest/cpp/Node.cpp
    ../../androidTest/cpp/NodeList.cpp
    ../../androidTest/cpp/minidom.cpp
  )
endif()

add_library( liquidjs SHARED ${SOURCES} )

add_library(v8_base STATIC IMPORTED)
set_target_properties(
        v8_base PROPERTIES IMPORTED_LOCATION
        ${PROJECT_SOURCE_DIR}/../../../prebuilt/${ANDROID_ABI}/libv8_base.a )
add_library(v8_libbase STATIC IMPORTED)
set_target_properties(
        v8_libbase PROPERTIES IMPORTED_LOCATION
        ${PROJECT_SOURCE_DIR}/../../../prebuilt/${ANDROID_ABI}/libv8_libbase.a )
add_library(v8_snapshot STATIC IMPORTED)
set_target_properties(
        v8_snapshot PROPERTIES IMPORTED_LOCATION
        ${PROJECT_SOURCE_DIR}/../../../prebuilt/${ANDROID_ABI}/libv8_snapshot.a )
add_library(v8_libsampler STATIC IMPORTED)
set_target_properties(
        v8_libsampler PROPERTIES IMPORTED_LOCATION
        ${PROJECT_SOURCE_DIR}/../../../prebuilt/${ANDROID_ABI}/libv8_libsampler.a )
add_library(v8_libplatform STATIC IMPORTED)
set_target_properties(
        v8_libplatform PROPERTIES IMPORTED_LOCATION
        ${PROJECT_SOURCE_DIR}/../../../prebuilt/${ANDROID_ABI}/libv8_libplatform.a )

add_library(icudata STATIC IMPORTED)
set_target_properties(
		icudata PROPERTIES IMPORTED_LOCATION
		${PROJECT_SOURCE_DIR}/../../../prebuilt/${ANDROID_ABI}/libicudata.a )

include_directories(
  ./
  ../../../../deps/JavaScriptCore/include
  ../../../../deps/node-10.15.3/deps/v8
  ../../../../deps/node-10.15.3/deps/v8/include
  ../../../../deps/node-10.15.3/deps/uv/include
  ../../../../deps/node-10.15.3/deps/uv/src
  ../../../../deps/utfcpp
  ../../../../deps/boost_1_66_0
  ../../../../deps/node-10.15.3/deps/icu-small/source/i18n
  ../../../../deps/node-10.15.3/deps/icu-small/source/common
)

find_library( log-lib log )

unset(DEFS_RELEASE)
unset(CFLAGS_RELEASE)

list (APPEND DEFS_RELEASE
  -DV8_DEPRECATION_WARNINGS=1
  -DNODE_USE_V8_PLATFORM=1
  -DHAVE_INSPECTOR=0
  -DV8_INSPECTOR_USE_STL=1
  -DV8_INSPECTOR_USE_OLD_STL=1
  -D__POSIX__
  -D_LARGEFILE_SOURCE
  -D_GLIBCXX_USE_C99_MATH
  -D_REENTRANT=1
  -D_FILE_OFFSET_BITS=64
  -DU_I18N_IMPLEMENTATION=1
  -DU_ATTRIBUTE_DEPRECATED=
  -D_CRT_SECURE_NO_DEPRECATE=
  -DU_STATIC_IMPLEMENTATION=1
  -DUCONFIG_NO_SERVICE=1
  -DUCONFIG_NO_REGULAR_EXPRESSIONS=1
  -DU_ENABLE_DYLOAD=0
  -DU_HAVE_STD_STRING=1
  -DUCONFIG_NO_BREAK_ITERATION=0
  -DU_COMMON_IMPLEMENTATION=1
)

list (APPEND CFLAGS_RELEASE
  -Wall
  -Wextra
  -Wno-unused-parameter
  -Wno-unused-function
  -fPIC
  -O3
  -fno-omit-frame-pointer
  -fPIE
  -Wno-strict-aliasing
  -Wno-unused-variable
  -Wno-mismatched-tags
  -Wno-unused-result
  -Wno-sign-compare
  -Wno-implicit-function-declaration
)

list (APPEND CPPFLAGS_RELEASE
    -std=c++1y
)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    list (APPEND CFLAGS_RELEASE -DDEBUG)
endif()

string (REPLACE ";" " " C_FLAGS_STR "${CFLAGS_RELEASE} ${DEFS_RELEASE}")
SET( CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} ${C_FLAGS_STR}" )

string (REPLACE ";" " " CPP_FLAGS_STR "${CFLAGS_RELEASE} ${CPPFLAGS_RELEASE} ${DEFS_RELEASE}")
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CPP_FLAGS_STR}" )

target_link_libraries( liquidjs v8_libplatform v8_base v8_libbase v8_snapshot v8_libsampler icudata ${log-lib} )
