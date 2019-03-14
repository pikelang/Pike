#pike __REAL_VERSION__

//
// experimental module to read CIFF data
// Canon "Camera Image File Format"
//

constant kStgFormatMask=0xc000; 
constant kDataTypeMask=0x3800; 
constant kIDCodeMask=0x07ff; 
constant kTypeIDCodeMask=0x3fff; 
constant kStg_InHeapSpace=0x0000; 
constant kStg_InRecordEntry=0x4000; 
constant kDT_BYTE=0x0000; 
constant kDT_ASCII=0x0800; 
constant kDT_WORD=0x1000; 
constant kDT_DWORD=0x1800; 
constant kDT_BYTE2=0x2000; 
constant kDT_HeapTypeProperty1=0x2800; 
constant kDT_HeapTypeProperty2=0x3000; 
constant kTC_WildCard=0xffff; 
constant kTC_Null=0x0000; /* null record */ 
constant kTC_Free=0x0001; /* free record */ 
constant kTC_ExUsed=0x0002; //special type for implementation purpose*/ 
constant kTC_Description=(kDT_ASCII | 0x0005); 
constant kTC_ModelName=(kDT_ASCII | 0x000a); 
constant kTC_FirmwareVersion=(kDT_ASCII | 0x000b); 
constant kTC_ComponentVersion=(kDT_ASCII | 0x000c); 
constant kTC_ROMOperationMode=(kDT_ASCII | 0x000d); 
constant kTC_OwnerName=(kDT_ASCII | 0x0010); 
constant kTC_ImageFileName=(kDT_ASCII | 0x0016); 
constant kTC_ThumbnailFileName=(kDT_ASCII | 0x0017); 
constant kTC_TargetImageType=(kDT_WORD | 0x000a); 
constant kTC_SR_ReleaseMethod=(kDT_WORD | 0x0010); 
constant kTC_SR_ReleaseTiming=(kDT_WORD | 0x0011); 
constant kTC_ReleaseSetting=(kDT_WORD | 0x0016); 
constant kTC_BodySensitivity=(kDT_WORD | 0x001c); 
constant kTC_ImageFormat=(kDT_DWORD | 0x0003); 
constant kTC_RecordID=(kDT_DWORD | 0x0004); 
constant kTC_SelfTimerTime=(kDT_DWORD | 0x0006); 
constant kTC_SR_TargetDistanceSetting=(kDT_DWORD | 0x0007);
constant kTC_BodyID=(kDT_DWORD | 0x000b); 
constant kTC_CapturedTime=(kDT_DWORD | 0x000e); 
constant kTC_ImageSpec=(kDT_DWORD | 0x0010); 
constant kTC_SR_EF=(kDT_DWORD | 0x0013); 
constant kTC_MI_EV=(kDT_DWORD | 0x0014); 
constant kTC_SerialNumber=(kDT_DWORD | 0x0017); 
constant kTC_SR_Exposure=(kDT_DWORD | 0x0018); 
constant kTC_CameraObject=(0x0007 | kDT_HeapTypeProperty1); 
constant kTC_ShootingRecord=(0x0002 | kDT_HeapTypeProperty2);
constant kTC_MeasuredInfo=(0x0003 | kDT_HeapTypeProperty2);
constant kTC_CameraSpecificaiton=(0x0004 | kDT_HeapTypeProperty2);

// custom added
constant kTC_d30_JPEG_preview=0x2007;
constant kTC_d30_sensor_data=0x2005;
constant kTC_d30_Exif=0x300a;

constant _stgtoid=([
   0x0000:"kStg_InHeapSpace", 
   0x4000:"kStg_InRecordEntry", 
]);

constant _ctoid=
([
   0x0000:"kDT_BYTE", 
   0x0800:"kDT_ASCII", 
   0x1000:"kDT_WORD", 
   0x1800:"kDT_DWORD", 
   0x2000:"kDT_BYTE2", 
   0xffff:"kTC_WildCard", 
   0x0000:"kTC_Null", /* null record */ 
   0x0001:"kTC_Free", /* free record */ 
   0x0002:"kTC_ExUsed", //special type for implementation purpose*/ 
]);

constant _dttoid=([
   (kDT_ASCII | 0x0005):"kTC_Description", 
   (kDT_ASCII | 0x000a):"kTC_ModelName", 
   (kDT_ASCII | 0x000b):"kTC_FirmwareVersion", 
   (kDT_ASCII | 0x000c):"kTC_ComponentVersion", 
   (kDT_ASCII | 0x000d):"kTC_ROMOperationMode", 
   (kDT_ASCII | 0x0010):"kTC_OwnerName", 
   (kDT_ASCII | 0x0016):"kTC_ImageFileName", 
   (kDT_ASCII | 0x0017):"kTC_ThumbnailFileName", 
   (kDT_WORD | 0x000a):"kTC_TargetImageType", 
   (kDT_WORD | 0x0010):"kTC_SR_ReleaseMethod", 
   (kDT_WORD | 0x0011):"kTC_SR_ReleaseTiming", 
   (kDT_WORD | 0x0016):"kTC_ReleaseSetting", 
   (kDT_WORD | 0x001c):"kTC_BodySensitivity", 
   (kDT_DWORD | 0x0003):"kTC_ImageFormat", 
   (kDT_DWORD | 0x0004):"kTC_RecordID", 
   (kDT_DWORD | 0x0006):"kTC_SelfTimerTime", 
   (kDT_DWORD | 0x0007):"kTC_SR_TargetDistanceSetting",
   (kDT_DWORD | 0x000b):"kTC_BodyID", 
   (kDT_DWORD | 0x000e):"kTC_CapturedTime", 
   (kDT_DWORD | 0x0010):"kTC_ImageSpec", 
   (kDT_DWORD | 0x0013):"kTC_SR_EF", 
   (kDT_DWORD | 0x0014):"kTC_MI_EV", 
   (kDT_DWORD | 0x0017):"kTC_SerialNumber", 
   (kDT_DWORD | 0x0018):"kTC_SR_Exposure", 
   (0x0007 | kDT_HeapTypeProperty1):"kTC_CameraObject", 
   (0x0002 | kDT_HeapTypeProperty2):"kTC_ShootingRecord",
   (0x0003 | kDT_HeapTypeProperty2):"kTC_MeasuredInfo",
   (0x0004 | kDT_HeapTypeProperty2):"kTC_CameraSpecificaiton",

// custom added

   0x2007: "kTC_d30_JPEG_preview",
   0x2005: "kTC_d30_sensor_data",
   0x300a: "kTC_d30_Exif",
]);
