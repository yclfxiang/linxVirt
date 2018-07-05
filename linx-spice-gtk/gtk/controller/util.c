/* util.c generated by valac 0.30.1, the Vala compiler
 * generated from util.vala, do not modify */

/* Copyright (C) 2012 Red Hat, Inc.*/
/* This library is free software; you can redistribute it and/or*/
/* modify it under the terms of the GNU Lesser General Public*/
/* License as published by the Free Software Foundation; either*/
/* version 2.1 of the License, or (at your option) any later version.*/
/* This library is distributed in the hope that it will be useful,*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU*/
/* Lesser General Public License for more details.*/
/* You should have received a copy of the GNU Lesser General Public*/
/* License along with this library; if not, see <http://www.gnu.org/licenses/>.*/

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
typedef struct _SpiceCtrlInputStreamReadData SpiceCtrlInputStreamReadData;
typedef struct _SpiceCtrlOutputStreamWriteData SpiceCtrlOutputStreamWriteData;

struct _SpiceCtrlInputStreamReadData {
	int _state_;
	GObject* _source_object_;
	GAsyncResult* _res_;
	GSimpleAsyncResult* _async_result;
	GInputStream* stream;
	guint8* buffer;
	gint buffer_length1;
	gint length;
	guint8* _tmp0_;
	gint _tmp0__length1;
	gssize i;
	gssize _tmp1_;
	gint _tmp2_;
	gssize n;
	GInputStream* _tmp3_;
	guint8* _tmp4_;
	gint _tmp4__length1;
	gssize _tmp5_;
	gint _tmp6_;
	gssize _tmp7_;
	gssize _tmp8_;
	GError* _tmp9_;
	gssize _tmp10_;
	gssize _tmp11_;
	GError * _inner_error_;
};

struct _SpiceCtrlOutputStreamWriteData {
	int _state_;
	GObject* _source_object_;
	GAsyncResult* _res_;
	GSimpleAsyncResult* _async_result;
	GOutputStream* stream;
	guint8* buffer;
	gint buffer_length1;
	gint length;
	guint8* _tmp0_;
	gint _tmp0__length1;
	gssize i;
	gssize _tmp1_;
	gint _tmp2_;
	gssize n;
	GOutputStream* _tmp3_;
	guint8* _tmp4_;
	gint _tmp4__length1;
	gssize _tmp5_;
	gint _tmp6_;
	gssize _tmp7_;
	gssize _tmp8_;
	GError* _tmp9_;
	gssize _tmp10_;
	gssize _tmp11_;
	GError * _inner_error_;
};



static void spice_ctrl_input_stream_read_data_free (gpointer _data);
void spice_ctrl_input_stream_read (GInputStream* stream, guint8* buffer, int buffer_length1, GAsyncReadyCallback _callback_, gpointer _user_data_);
void spice_ctrl_input_stream_read_finish (GAsyncResult* _res_, GError** error);
static gboolean spice_ctrl_input_stream_read_co (SpiceCtrlInputStreamReadData* _data_);
static void spice_ctrl_input_stream_read_ready (GObject* source_object, GAsyncResult* _res_, gpointer _user_data_);
static void spice_ctrl_output_stream_write_data_free (gpointer _data);
void spice_ctrl_output_stream_write (GOutputStream* stream, guint8* buffer, int buffer_length1, GAsyncReadyCallback _callback_, gpointer _user_data_);
void spice_ctrl_output_stream_write_finish (GAsyncResult* _res_, GError** error);
static gboolean spice_ctrl_output_stream_write_co (SpiceCtrlOutputStreamWriteData* _data_);
static void spice_ctrl_output_stream_write_ready (GObject* source_object, GAsyncResult* _res_, gpointer _user_data_);


static void spice_ctrl_input_stream_read_data_free (gpointer _data) {
	SpiceCtrlInputStreamReadData* _data_;
	_data_ = _data;
	_g_object_unref0 (_data_->stream);
	g_slice_free (SpiceCtrlInputStreamReadData, _data_);
}


static gpointer _g_object_ref0 (gpointer self) {
	return self ? g_object_ref (self) : NULL;
}


void spice_ctrl_input_stream_read (GInputStream* stream, guint8* buffer, int buffer_length1, GAsyncReadyCallback _callback_, gpointer _user_data_) {
	SpiceCtrlInputStreamReadData* _data_;
	GInputStream* _tmp0_ = NULL;
	GInputStream* _tmp1_ = NULL;
	guint8* _tmp2_ = NULL;
	gint _tmp2__length1 = 0;
	_data_ = g_slice_new0 (SpiceCtrlInputStreamReadData);
	_data_->_async_result = g_simple_async_result_new (NULL, _callback_, _user_data_, spice_ctrl_input_stream_read);
	g_simple_async_result_set_op_res_gpointer (_data_->_async_result, _data_, spice_ctrl_input_stream_read_data_free);
	_tmp0_ = stream;
	_tmp1_ = _g_object_ref0 (_tmp0_);
	_g_object_unref0 (_data_->stream);
	_data_->stream = _tmp1_;
	_tmp2_ = buffer;
	_tmp2__length1 = buffer_length1;
	_data_->buffer = _tmp2_;
	_data_->buffer_length1 = _tmp2__length1;
	spice_ctrl_input_stream_read_co (_data_);
}


void spice_ctrl_input_stream_read_finish (GAsyncResult* _res_, GError** error) {
	SpiceCtrlInputStreamReadData* _data_;
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (_res_), error)) {
		return;
	}
	_data_ = g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (_res_));
}


static void spice_ctrl_input_stream_read_ready (GObject* source_object, GAsyncResult* _res_, gpointer _user_data_) {
	SpiceCtrlInputStreamReadData* _data_;
	_data_ = _user_data_;
	_data_->_source_object_ = source_object;
	_data_->_res_ = _res_;
	spice_ctrl_input_stream_read_co (_data_);
}


static gboolean spice_ctrl_input_stream_read_co (SpiceCtrlInputStreamReadData* _data_) {
	switch (_data_->_state_) {
		case 0:
		goto _state_0;
		case 1:
		goto _state_1;
		default:
		g_assert_not_reached ();
	}
	_state_0:
	_data_->_tmp0_ = NULL;
	_data_->_tmp0__length1 = 0;
	_data_->_tmp0_ = _data_->buffer;
	_data_->_tmp0__length1 = _data_->buffer_length1;
	_data_->length = _data_->_tmp0__length1;
	_data_->i = (gssize) 0;
	while (TRUE) {
		_data_->_tmp1_ = 0L;
		_data_->_tmp1_ = _data_->i;
		_data_->_tmp2_ = 0;
		_data_->_tmp2_ = _data_->length;
		if (!(_data_->_tmp1_ < ((gssize) _data_->_tmp2_))) {
			break;
		}
		_data_->_tmp3_ = NULL;
		_data_->_tmp3_ = _data_->stream;
		_data_->_tmp4_ = NULL;
		_data_->_tmp4__length1 = 0;
		_data_->_tmp4_ = _data_->buffer;
		_data_->_tmp4__length1 = _data_->buffer_length1;
		_data_->_tmp5_ = 0L;
		_data_->_tmp5_ = _data_->i;
		_data_->_tmp6_ = 0;
		_data_->_tmp6_ = _data_->length;
		_data_->_state_ = 1;
		g_input_stream_read_async (_data_->_tmp3_, _data_->_tmp4_ + ((gint) _data_->_tmp5_), (gsize) (_data_->_tmp6_ - ((gint) _data_->_tmp5_)), G_PRIORITY_DEFAULT, NULL, spice_ctrl_input_stream_read_ready, _data_);
		return FALSE;
		_state_1:
		_data_->_tmp7_ = 0L;
		_data_->_tmp7_ = g_input_stream_read_finish (_data_->_tmp3_, _data_->_res_, &_data_->_inner_error_);
		_data_->n = _data_->_tmp7_;
		if (G_UNLIKELY (_data_->_inner_error_ != NULL)) {
			if (_data_->_inner_error_->domain == G_IO_ERROR) {
				g_simple_async_result_set_from_error (_data_->_async_result, _data_->_inner_error_);
				g_error_free (_data_->_inner_error_);
				if (_data_->_state_ == 0) {
					g_simple_async_result_complete_in_idle (_data_->_async_result);
				} else {
					g_simple_async_result_complete (_data_->_async_result);
				}
				g_object_unref (_data_->_async_result);
				return FALSE;
			} else {
				g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _data_->_inner_error_->message, g_quark_to_string (_data_->_inner_error_->domain), _data_->_inner_error_->code);
				g_clear_error (&_data_->_inner_error_);
				return FALSE;
			}
		}
		_data_->_tmp8_ = 0L;
		_data_->_tmp8_ = _data_->n;
		if (_data_->_tmp8_ == ((gssize) 0)) {
			_data_->_tmp9_ = NULL;
			_data_->_tmp9_ = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CLOSED, "closed stream");
			_data_->_inner_error_ = _data_->_tmp9_;
			if (_data_->_inner_error_->domain == G_IO_ERROR) {
				g_simple_async_result_set_from_error (_data_->_async_result, _data_->_inner_error_);
				g_error_free (_data_->_inner_error_);
				if (_data_->_state_ == 0) {
					g_simple_async_result_complete_in_idle (_data_->_async_result);
				} else {
					g_simple_async_result_complete (_data_->_async_result);
				}
				g_object_unref (_data_->_async_result);
				return FALSE;
			} else {
				g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _data_->_inner_error_->message, g_quark_to_string (_data_->_inner_error_->domain), _data_->_inner_error_->code);
				g_clear_error (&_data_->_inner_error_);
				return FALSE;
			}
		}
		_data_->_tmp10_ = 0L;
		_data_->_tmp10_ = _data_->i;
		_data_->_tmp11_ = 0L;
		_data_->_tmp11_ = _data_->n;
		_data_->i = _data_->_tmp10_ + _data_->_tmp11_;
	}
	if (_data_->_state_ == 0) {
		g_simple_async_result_complete_in_idle (_data_->_async_result);
	} else {
		g_simple_async_result_complete (_data_->_async_result);
	}
	g_object_unref (_data_->_async_result);
	return FALSE;
}


static void spice_ctrl_output_stream_write_data_free (gpointer _data) {
	SpiceCtrlOutputStreamWriteData* _data_;
	_data_ = _data;
	_g_object_unref0 (_data_->stream);
	_data_->buffer = (g_free (_data_->buffer), NULL);
	g_slice_free (SpiceCtrlOutputStreamWriteData, _data_);
}


void spice_ctrl_output_stream_write (GOutputStream* stream, guint8* buffer, int buffer_length1, GAsyncReadyCallback _callback_, gpointer _user_data_) {
	SpiceCtrlOutputStreamWriteData* _data_;
	GOutputStream* _tmp0_ = NULL;
	GOutputStream* _tmp1_ = NULL;
	_data_ = g_slice_new0 (SpiceCtrlOutputStreamWriteData);
	_data_->_async_result = g_simple_async_result_new (NULL, _callback_, _user_data_, spice_ctrl_output_stream_write);
	g_simple_async_result_set_op_res_gpointer (_data_->_async_result, _data_, spice_ctrl_output_stream_write_data_free);
	_tmp0_ = stream;
	_tmp1_ = _g_object_ref0 (_tmp0_);
	_g_object_unref0 (_data_->stream);
	_data_->stream = _tmp1_;
	_data_->buffer = (g_free (_data_->buffer), NULL);
	_data_->buffer = buffer;
	_data_->buffer_length1 = buffer_length1;
	spice_ctrl_output_stream_write_co (_data_);
}


void spice_ctrl_output_stream_write_finish (GAsyncResult* _res_, GError** error) {
	SpiceCtrlOutputStreamWriteData* _data_;
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (_res_), error)) {
		return;
	}
	_data_ = g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (_res_));
}


static void spice_ctrl_output_stream_write_ready (GObject* source_object, GAsyncResult* _res_, gpointer _user_data_) {
	SpiceCtrlOutputStreamWriteData* _data_;
	_data_ = _user_data_;
	_data_->_source_object_ = source_object;
	_data_->_res_ = _res_;
	spice_ctrl_output_stream_write_co (_data_);
}


static gboolean spice_ctrl_output_stream_write_co (SpiceCtrlOutputStreamWriteData* _data_) {
	switch (_data_->_state_) {
		case 0:
		goto _state_0;
		case 1:
		goto _state_1;
		default:
		g_assert_not_reached ();
	}
	_state_0:
	_data_->_tmp0_ = NULL;
	_data_->_tmp0__length1 = 0;
	_data_->_tmp0_ = _data_->buffer;
	_data_->_tmp0__length1 = _data_->buffer_length1;
	_data_->length = _data_->_tmp0__length1;
	_data_->i = (gssize) 0;
	while (TRUE) {
		_data_->_tmp1_ = 0L;
		_data_->_tmp1_ = _data_->i;
		_data_->_tmp2_ = 0;
		_data_->_tmp2_ = _data_->length;
		if (!(_data_->_tmp1_ < ((gssize) _data_->_tmp2_))) {
			break;
		}
		_data_->_tmp3_ = NULL;
		_data_->_tmp3_ = _data_->stream;
		_data_->_tmp4_ = NULL;
		_data_->_tmp4__length1 = 0;
		_data_->_tmp4_ = _data_->buffer;
		_data_->_tmp4__length1 = _data_->buffer_length1;
		_data_->_tmp5_ = 0L;
		_data_->_tmp5_ = _data_->i;
		_data_->_tmp6_ = 0;
		_data_->_tmp6_ = _data_->length;
		_data_->_state_ = 1;
		g_output_stream_write_async (_data_->_tmp3_, _data_->_tmp4_ + ((gint) _data_->_tmp5_), (gsize) (_data_->_tmp6_ - ((gint) _data_->_tmp5_)), G_PRIORITY_DEFAULT, NULL, spice_ctrl_output_stream_write_ready, _data_);
		return FALSE;
		_state_1:
		_data_->_tmp7_ = 0L;
		_data_->_tmp7_ = g_output_stream_write_finish (_data_->_tmp3_, _data_->_res_, &_data_->_inner_error_);
		_data_->n = _data_->_tmp7_;
		if (G_UNLIKELY (_data_->_inner_error_ != NULL)) {
			if (_data_->_inner_error_->domain == G_IO_ERROR) {
				g_simple_async_result_set_from_error (_data_->_async_result, _data_->_inner_error_);
				g_error_free (_data_->_inner_error_);
				_data_->buffer = (g_free (_data_->buffer), NULL);
				if (_data_->_state_ == 0) {
					g_simple_async_result_complete_in_idle (_data_->_async_result);
				} else {
					g_simple_async_result_complete (_data_->_async_result);
				}
				g_object_unref (_data_->_async_result);
				return FALSE;
			} else {
				_data_->buffer = (g_free (_data_->buffer), NULL);
				g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _data_->_inner_error_->message, g_quark_to_string (_data_->_inner_error_->domain), _data_->_inner_error_->code);
				g_clear_error (&_data_->_inner_error_);
				return FALSE;
			}
		}
		_data_->_tmp8_ = 0L;
		_data_->_tmp8_ = _data_->n;
		if (_data_->_tmp8_ == ((gssize) 0)) {
			_data_->_tmp9_ = NULL;
			_data_->_tmp9_ = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CLOSED, "closed stream");
			_data_->_inner_error_ = _data_->_tmp9_;
			if (_data_->_inner_error_->domain == G_IO_ERROR) {
				g_simple_async_result_set_from_error (_data_->_async_result, _data_->_inner_error_);
				g_error_free (_data_->_inner_error_);
				_data_->buffer = (g_free (_data_->buffer), NULL);
				if (_data_->_state_ == 0) {
					g_simple_async_result_complete_in_idle (_data_->_async_result);
				} else {
					g_simple_async_result_complete (_data_->_async_result);
				}
				g_object_unref (_data_->_async_result);
				return FALSE;
			} else {
				_data_->buffer = (g_free (_data_->buffer), NULL);
				g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _data_->_inner_error_->message, g_quark_to_string (_data_->_inner_error_->domain), _data_->_inner_error_->code);
				g_clear_error (&_data_->_inner_error_);
				return FALSE;
			}
		}
		_data_->_tmp10_ = 0L;
		_data_->_tmp10_ = _data_->i;
		_data_->_tmp11_ = 0L;
		_data_->_tmp11_ = _data_->n;
		_data_->i = _data_->_tmp10_ + _data_->_tmp11_;
	}
	_data_->buffer = (g_free (_data_->buffer), NULL);
	if (_data_->_state_ == 0) {
		g_simple_async_result_complete_in_idle (_data_->_async_result);
	} else {
		g_simple_async_result_complete (_data_->_async_result);
	}
	g_object_unref (_data_->_async_result);
	return FALSE;
}



