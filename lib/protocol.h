// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

/*
 * Protocol version and types
 */
#define TSQ_PROTOCOL_VERSION           1

#define TSQ_PROTOCOL_REJECT            0
#define TSQ_PROTOCOL_TERM              1
#define TSQ_PROTOCOL_RAW               2
#define TSQ_PROTOCOL_CLIENTFD          3
#define TSQ_PROTOCOL_TERM_SERVER       4
#define TSQ_PROTOCOL_RAW_SERVER        5
#define TSQ_PROTOCOL_TERM_SERVERFD     6
#define TSQ_PROTOCOL_RAW_SERVERFD      7

/*
 * Disconnect/exit codes
 */
#define TSQ_STATUS_NORMAL              0
#define TSQ_STATUS_CLOSED              1
#define TSQ_STATUS_SERVER_SHUTDOWN     2
#define TSQ_STATUS_FORWARDER_SHUTDOWN  3
#define TSQ_STATUS_SERVER_ERROR        4
#define TSQ_STATUS_FORWARDER_ERROR     5
#define TSQ_STATUS_PROTOCOL_MISMATCH   6
#define TSQ_STATUS_PROTOCOL_ERROR      7
#define TSQ_STATUS_DUPLICATE_CONN      8
#define TSQ_STATUS_LOST_CONN           9
#define TSQ_STATUS_CONN_LIMIT_REACHED  10
#define TSQ_STATUS_IDLE_TIMEOUT        11
#define TSQ_FLAG_PROXY_CLOSED          0x80000000

/*
 * Command type mask and codes
 */
#define TSQ_CMDTYPE_MASK               0xff000000
#define TSQ_CMDTYPE_PLAIN              0
#define _P(x) (TSQ_CMDTYPE_PLAIN|x)
#define TSQ_CMDTYPE_SERVER             0x01000000
#define _S(x) (TSQ_CMDTYPE_SERVER|x)
#define TSQ_CMDTYPE_CLIENT             0x02000000
#define _C(x) (TSQ_CMDTYPE_CLIENT|x)
#define TSQ_CMDTYPE_TERM               0x03000000
#define _T(x) (TSQ_CMDTYPE_TERM|x)

/*
 * Empty commands
 */
// OUT
#define TSQ_HANDSHAKE_COMPLETE                             _P(1)

// OUT serverid termid version hops nterms key+value...
#define TSQ_ANNOUNCE_SERVER                                _P(2)

// OUT termid serverid hops width height key+value...
#define TSQ_ANNOUNCE_TERM                                  _P(3)

// OUT connid serverid hops key+value...
#define TSQ_ANNOUNCE_CONN                                  _P(4)

// IN code | OUT code
#define TSQ_DISCONNECT                                     _P(5)

// IN | OUT
#define TSQ_KEEPALIVE                                      _P(6)

// IN timeout
#define TSQ_CONFIGURE_KEEPALIVE                            _P(7)

// IN hopid
#define TSQ_TASK_RESUME                                    _P(8)

// IN data
#define TSQ_DISCARD                                        _P(9)

/*
 * Server commands
 */
// IN serverid clientid | RESP clientid serverid time8
#define TSQ_GET_SERVER_TIME                                _S(1000)
#define TSQ_GET_SERVER_TIME_RESPONSE                       _C(1000)

// IN serverid clientid | RESP clientid serverid key+value...
#define TSQ_GET_SERVER_ATTRIBUTES                          _S(1001)
#define TSQ_GET_SERVER_ATTRIBUTES_RESPONSE                 _C(1001)

// IN serverid clientid key... | OUT serverid key[+value] | RESP clientid +OUT
#define TSQ_GET_SERVER_ATTRIBUTE                           _S(1002)
#define TSQ_GET_SERVER_ATTRIBUTE_RESPONSE                  _C(1002)

// IN serverid clientid key+value...
#define TSQ_SET_SERVER_ATTRIBUTE                           _S(1003)

// IN serverid clientid key...
#define TSQ_REMOVE_SERVER_ATTRIBUTE                        _S(1004)

// OUT serverid code
#define TSQ_REMOVE_SERVER                                  _S(1005)

// IN serverid clientid termid width height key+value...
#define TSQ_CREATE_TERM                                    _S(1006)

// IN serverid clientid taskid hopid
#define TSQ_TASK_PAUSE                                     _S(1007)

// IN serverid clientid taskid data | RESP clientid serverid taskid data
#define TSQ_TASK_INPUT                                     _S(1008)
#define TSQ_TASK_OUTPUT                                    _C(1008)

// IN serverid clientid taskid answer | RESP clientid serverid taskid question
#define TSQ_TASK_ANSWER                                    _S(1009)
#define TSQ_TASK_QUESTION                                  _C(1009)

// IN serverid clientid taskid
#define TSQ_CANCEL_TASK                                    _S(1010)

// IN serverid clientid taskid chunksize mode config name
// INPUT data|empty
// OUTPUT Acking:bytes8 Starting:bytes8 newname Finished:bytes8 Error:bytes8 code errstr
#define TSQ_UPLOAD_FILE                                    _S(1011)

// IN serverid clientid taskid chunksize windowsize name
// INPUT bytes8
// OUTPUT Starting:mode size8 Running:(data|empty) Error:code errstr
#define TSQ_DOWNLOAD_FILE                                  _S(1012)

// IN serverid clientid taskid config name
// OUTPUT Finished: Error:code errstr
#define TSQ_DELETE_FILE                                    _S(1013)

// IN serverid clientid taskid config name dest
// OUTPUT Finished: Error:code errstr
#define TSQ_RENAME_FILE                                    _S(1014)

// IN serverid clientid taskid chunksize mode
// INPUT data|empty
// OUTPUT Acking:bytes8 Starting:bytes8 newname Finished:bytes8 Error:bytes8 code errstr
#define TSQ_UPLOAD_PIPE                                    _S(1015)

// IN serverid clientid taskid chunksize windowsize mode
// INPUT bytes8
// OUTPUT Starting:newname Running:(data|empty) Error:code errstr
#define TSQ_DOWNLOAD_PIPE                                  _S(1016)

// IN serverid clientid taskid chunksize windowsize type addr
// INPUT Acking:bytes8 Running:connid (data|empty) Starting:connid
// OUTPUT Acking:bytes8 Running:connid (data|empty) Error:code errstr
#define TSQ_CONNECTING_PORTFWD                             _S(1017)

// IN serverid clientid taskid chunksize windowsize type addr
// INPUT Acking:bytes8 Running:connid (data|empty)
// OUTPUT Acking:bytes8 Running:connid (data|empty) Error:code errstr Starting:connid raddr
#define TSQ_LISTENING_PORTFWD                              _S(1018)

// IN serverid clientid taskid chunksize windowsize key+value...
// INPUT Acking:bytes8 Running:data|empty
// OUTPUT Acking:bytes8 Running:data Starting:pid Finished/Error:outcome code/status msg
#define TSQ_RUN_COMMAND                                    _S(1019)
// Outcome is a value from Tsq::TermOutcome
// If Tsq::TermRunning, code/status is a task error code
// Otherwise, code/status is the exit status of the command

// IN serverid clientid taskid key+value...
// INPUT data|empty
// OUTPUT Starting:pid Running:data Finished:connid Error:code subcode errstr
#define TSQ_RUN_CONNECT                                    _S(1020)

// IN serverid clientid taskid name
// INPUT Running:id op name <params>
// OUTPUT Running:id code <params> Starting:flags Error:code errstr
#define TSQ_MOUNT_FILE_READWRITE                           _S(1021)
#define TSQ_MOUNT_FILE_READONLY                            _S(1022)

// IN serverid clientid data
#define TSQ_MONITOR_INPUT                                  _S(1023)

/*
 * Client commands
 */
// IN clientid version hops flags key+value...
#define TSQ_ANNOUNCE_CLIENT                                _C(2000)

// IN clientid
#define TSQ_REMOVE_CLIENT                                  _C(2001)

// IN serverid clientid targetid key... | RESP clientid serverid targetid key[+value]
#define TSQ_GET_CLIENT_ATTRIBUTE                           _S(2002)
#define TSQ_GET_CLIENT_ATTRIBUTE_RESPONSE                  _C(2002)

// OUT clientid destid termid size8 threshold8
#define TSQ_THROTTLE_PAUSE                                 _C(2005)

// OUT termid
#define TSQ_THROTTLE_RESUME                                _T(2005)

/*
 * Terminal input/output
 */
// IN termid clientid data
#define TSQ_INPUT                                          _T(3000)
// IN termid clientid mouseflags x y
#define TSQ_MOUSE_INPUT                                    _T(3001)

// OUT termid | RESP clientid termid
#define TSQ_BEGIN_OUTPUT                                   _T(3000)
#define TSQ_BEGIN_OUTPUT_RESPONSE                          _C(3000)

// OUT termid flags8
#define TSQ_FLAGS_CHANGED                                  _T(3001)

// IN termid clientid caporder+bufid
// OUT termid rows8 caporder+bufid
#define TSQ_BUFFER_CAPACITY                                _T(3002)

// OUT termid rows8 bufid
#define TSQ_BUFFER_LENGTH                                  _T(3003)

// OUT termid bufid
#define TSQ_BUFFER_SWITCHED                                _T(3004)

// OUT termid size margins
#define TSQ_SIZE_CHANGED                                   _T(3005)

// OUT termid x y pos flags+subpos
#define TSQ_CURSOR_MOVED                                   _T(3006)

// OUT termid type count
#define TSQ_BELL_RANG                                      _T(3007)

// IN termid clientid start8 end8 bufid
// OUT termid rownum8 flags+bufid modtime nranges range... string
// RESP clientid +OUT
#define TSQ_ROW_CONTENT                                    _T(3008)
#define TSQ_ROW_CONTENT_RESPONSE                           _C(3008)

// OUT termid regid type+bufid flags parent srow8 erow8 scol ecol key+value...
// RESP clientid +OUT
#define TSQ_REGION_UPDATE                                  _T(3009)
#define TSQ_REGION_UPDATE_RESPONSE                         _C(3009)

// OUT termid time8 name key+value...
#define TSQ_DIRECTORY_UPDATE                               _T(3010)

// OUT termid mtime8 size8 mode uid gid name key+value...
#define TSQ_FILE_UPDATE                                    _T(3011)

// OUT termid mtime8 name
#define TSQ_FILE_REMOVED                                   _T(3012)

// OUT termid | RESP clientid termid
#define TSQ_END_OUTPUT                                     _T(3013)
#define TSQ_END_OUTPUT_RESPONSE                            _C(3013)

// OUT termid x y
#define TSQ_MOUSE_MOVED                                    _T(3014)

// IN termid clientid id8
// RESP clientid termid id8 data
#define TSQ_IMAGE_CONTENT                                  _T(3015)
#define TSQ_IMAGE_CONTENT_RESPONSE                         _C(3015)

// IN termid clientid taskid id8 chunksize windowsize
// INPUT bytes8
// OUTPUT Starting: mode size8 Running:(data|empty) Error:code errstr
#define TSQ_DOWNLOAD_IMAGE                                 _T(3016)

/*
 * Terminal management and metadata
 */
// IN termid clientid | RESP clientid termid key+value...
#define TSQ_GET_TERM_ATTRIBUTES                            _T(3100)
#define TSQ_GET_TERM_ATTRIBUTES_RESPONSE                   _C(3100)
#define TSQ_GET_CONN_ATTRIBUTES_RESPONSE                   _C(3101)

// IN termid clientid key... | OUT termid key[+value] | RESP clientid +OUT
#define TSQ_GET_TERM_ATTRIBUTE                             _T(3101)
#define TSQ_GET_CONN_ATTRIBUTE                             _T(3102)
#define TSQ_GET_TERM_ATTRIBUTE_RESPONSE                    _C(3103)
#define TSQ_GET_CONN_ATTRIBUTE_RESPONSE                    _C(3104)

// IN termid clientid key+value...
#define TSQ_SET_TERM_ATTRIBUTE                             _T(3102)

// IN termid clientid key...
#define TSQ_REMOVE_TERM_ATTRIBUTE                          _T(3103)

// IN termid clientid size
#define TSQ_RESIZE_TERM                                    _T(3104)

// IN termid clientid | OUT termid code
#define TSQ_REMOVE_TERM                                    _T(3105)

// OUT termid code
#define TSQ_REMOVE_CONN                                    _T(3106)

// IN termid clientid newid width height key+value...
#define TSQ_DUPLICATE_TERM                                 _T(3106)

// IN termid clientid flags
#define TSQ_RESET_TERM                                     _T(3107)

// IN termid clientid
#define TSQ_CHANGE_OWNER                                   _T(3108)

// IN termid clientid
#define TSQ_REQUEST_DISCONNECT                             _T(3109)

// IN termid clientid
#define TSQ_TOGGLE_SOFT_SCROLL_LOCK                        _T(3110)

// IN termid clientid signal
#define TSQ_SEND_SIGNAL                                    _T(3111)

/*
 * Regions
 */
// IN termid clientid bufid type srow8 erow8 scol ecol key+value...
#define TSQ_CREATE_REGION                                  _T(3200)

// IN termid clientid bufid regid
#define TSQ_GET_REGION                                     _T(3201)

// IN termid clientid bufid regid
#define TSQ_REMOVE_REGION                                  _T(3202)
