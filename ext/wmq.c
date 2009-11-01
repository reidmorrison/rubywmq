/*
 *  Copyright 2006 J. Reid Morrison. Dimension Solutions, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "wmq.h"

VALUE wmq_queue;
VALUE wmq_queue_manager;
VALUE wmq_message;
VALUE wmq_exception;

void Init_wmq() {
    VALUE wmq;

    wmq = rb_define_module("WMQ");

    wmq_queue_manager = rb_define_class_under(wmq, "QueueManager", rb_cObject);
    rb_define_alloc_func(wmq_queue_manager, QUEUE_MANAGER_alloc);
    rb_define_singleton_method(wmq_queue_manager, "connect", QueueManager_singleton_connect, -1); /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "initialize", QueueManager_initialize, 1);  /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "connect", QueueManager_connect, 0);        /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "disconnect", QueueManager_disconnect, 0);  /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "open_queue", QueueManager_open_queue, -1); /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "access_queue", QueueManager_open_queue, -1); /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "begin", QueueManager_begin, 0);            /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "commit", QueueManager_commit, 0);          /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "backout", QueueManager_backout, 0);        /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "put", QueueManager_put, 1);                /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "comp_code", QueueManager_comp_code, 0);    /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "reason_code", QueueManager_reason_code, 0); /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "reason", QueueManager_reason, 0);          /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "exception_on_error", QueueManager_exception_on_error, 0); /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "connected?", QueueManager_connected_q, 0); /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "name", QueueManager_name, 0);              /* in wmq_queue_manager.c */
    rb_define_method(wmq_queue_manager, "execute", QueueManager_execute, 1);        /* in wmq_queue_manager.c */

    wmq_queue = rb_define_class_under(wmq, "Queue", rb_cObject);
    rb_define_alloc_func(wmq_queue, QUEUE_alloc);
    rb_define_singleton_method(wmq_queue, "open", Queue_singleton_open, -1);        /* in wmq_queue.c */
    rb_define_method(wmq_queue, "initialize", Queue_initialize, 1);                 /* in wmq_queue.c */
    rb_define_method(wmq_queue, "open", Queue_open, 0);                             /* in wmq_queue.c */
    rb_define_method(wmq_queue, "close", Queue_close, 0);                           /* in wmq_queue.c */
    rb_define_method(wmq_queue, "put", Queue_put, 1);                               /* in wmq_queue.c */
    rb_define_method(wmq_queue, "get", Queue_get, 1);                               /* in wmq_queue.c */
    rb_define_method(wmq_queue, "each", Queue_each, -1);                            /* in wmq_queue.c */
    rb_define_method(wmq_queue, "name", Queue_name, 0);                             /* in wmq_queue.c */
    rb_define_method(wmq_queue, "comp_code", Queue_comp_code, 0);                   /* in wmq_queue.c */
    rb_define_method(wmq_queue, "reason_code", Queue_reason_code, 0);               /* in wmq_queue.c */
    rb_define_method(wmq_queue, "reason", Queue_reason, 0);                         /* in wmq_queue.c */
    rb_define_method(wmq_queue, "open?", Queue_open_q, 0);                          /* in wmq_queue.c */

    wmq_message = rb_define_class_under(wmq, "Message", rb_cObject);
    rb_define_method(wmq_message, "initialize", Message_initialize, -1);            /* in wmq_message.c */
    rb_define_method(wmq_message, "clear", Message_clear, 0);                       /* in wmq_message.c */

    /*
     * WMQException is thrown whenever an MQ operation fails and
     * exception_on_error is true
     */
    wmq_exception = rb_define_class_under(wmq, "WMQException", rb_eRuntimeError);

    /*
     * Initialize id fields
     */
    Message_id_init();
    Queue_id_init();
    QueueManager_id_init();
    QueueManager_selector_id_init();
    QueueManager_command_id_init();
    wmq_structs_id_init();

    rb_require("wmq/wmq_temp");
    rb_require("wmq/wmq_const");
}

/*
 * For client build when dynamic loading is not being used E.g. Not Windows or Solaris ...
 */
void Init_wmq_client() {
    Init_wmq();
}
