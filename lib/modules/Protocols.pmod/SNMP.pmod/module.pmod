//! SNMPv1 and v2c

#include "snmp_errors.h"
#include "snmp_globals.h"

//! Error tooBig
constant ERROR_TOOBIG=SNMP_ERR_TOOBIG;

//! Error noError
constant ERROR_NOERROR=SNMP_ERR_NOERROR;

//! Error noSuchName
constant ERROR_NOSUCHNAME=SNMP_ERR_NOSUCHNAME;

//! Error badValue
constant ERROR_BADVALUE=SNMP_ERR_BADVALUE;

//! Error readOnly
constant ERROR_READONLY=SNMP_ERR_READONLY;

//! Error genError
constant ERROR_GENERROR=SNMP_ERR_GENERROR;

//! PDU type Get
constant REQUEST_GET=SNMP_REQUEST_GET;

//! PDU type GetNext
constant REQUEST_GETNEXT=SNMP_REQUEST_GETNEXT;

//! PDU type Set
constant REQUEST_SET=SNMP_REQUEST_SET;

//! PDU type GetResponse
constant REQUEST_GET_RESPONSE=SNMP_REQUEST_GET_RESPONSE;

//! PDU type Trap
constant REQUEST_TRAP=SNMP_REQUEST_TRAP;

