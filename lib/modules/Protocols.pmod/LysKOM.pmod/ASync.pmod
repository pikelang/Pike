import .ProtocolTypes;

/* 0 async-new-text-old */
array decode_0(array what)
{
   return ({ (int) what[0], 
	     TextStatOld(@what[1..]) });
}

/* 5 async-new-name */
array decode_5(array what)
{
   return ({ (int) what[0], 
	     what[1],
	     what[2] });
}

/* 6 async-i-am-on */
array decode_6(array what)
{
   return ({ WhoInfo(@what) });
}

/* 7 async-sync-db */
array decode_7(array what)
{
   return ({ });
}

/* 8 async-leave-conf */
array decode_8(array what)
{
   return ({ (int)what[0] });
}

/* 9 async-login */
array decode_9(array what)
{
   return ({ (int)what[0],
             (int)what[1] });
}

/* 10 async-broadcast */
array decode_10(array what)
{
   return ({ (int)what[0],
             what[1] });
}

/* 11 async-rejected-connection */
array decode_11(array what)
{
   return ({ });
}

/* 12 async-send-message */
array decode_12(array what)
{
   return ({ (int)what[0],
	     (int)what[1],
             what[2] });
}

/* 13 async-async-logout */
array decode_13(array what)
{
   return ({ (int)what[0],
             (int)what[1] });
}

/* 14 async-deleted-text */
array decode_14(array what)
{
   return ({ (int)what[0],
             TextStat(@what[1..]) });
}

/* 15 async-new-text */
array decode_15(array what)
{
   return ({ (int)what[0],
             TextStat(@what[1..]) });
}

/* 16 async-new-recipient */
array decode_16(array what)
{
   return ({ (int)what[0],
	     (int)what[1],
             (array(int))what[3] });
}

/* 17 async-sub-recipient */
array decode_17(array what)
{
   return ({ (int)what[0],
	     (int)what[1],
             (array(int))what[3] });
}

/* 18 async-new-membership */
array decode_18(array what)
{
   return ({ (int)what[0],
	     (int)what[1] });
}

mapping name2no=
([
   "async-async-logout":13,
   "async-broadcast":10,
   "async-deleted-text":14,
   "async-deleted-text":14,
   "async-i-am-on":6,
   "async-leave-conf":8,
   "async-login":9,
   "async-new-membership":18,
   "async-new-name":5,
   "async-new-recipient":16,
   "async-new-text":15,
   "async-new-text-old":0,
   "async-rejected-connection":11,
   "async-send-message":12,
   "async-sub-recipient":17,
   "async-sync-db":7,
]);
