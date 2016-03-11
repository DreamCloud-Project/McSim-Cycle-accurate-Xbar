/*
 * xbar_sc_fifo_ports.h
 *
 *  Created on: 7 Dec 2015
 *      Author: effiong
 */

#ifndef XBAR_SC_FIFO_PORTS_H_
#define XBAR_SC_FIFO_PORTS_H_

#include "sysc/communication/sc_port.h"
#include "xbar_sc_fifo_ifs.h"

namespace dreamcloud {
namespace platform_sclib {
// ----------------------------------------------------------------------------
//  CLASS : xb_sc_fifo_in<T>
//
//  The sc_fifo<T> input port class.
// ----------------------------------------------------------------------------

template<class T>
class xb_sc_fifo_in: public sc_port<xb_sc_fifo_in_if<T>, 0, SC_ONE_OR_MORE_BOUND> {
public:

	// typedefs
	typedef T data_type;

	typedef xb_sc_fifo_in_if<data_type> if_type;
	typedef sc_port<if_type, 0, SC_ONE_OR_MORE_BOUND> base_type;
	typedef xb_sc_fifo_in<data_type> this_type;

	typedef if_type in_if_type;
	typedef sc_port_b<in_if_type> in_port_type;

public:

	// constructors

	xb_sc_fifo_in() :
			base_type() {
	}

	explicit xb_sc_fifo_in(const char* name_) :
			base_type(name_) {
	}

	explicit xb_sc_fifo_in(in_if_type& interface_) :
			base_type(interface_) {
	}

	xb_sc_fifo_in(const char* name_, in_if_type& interface_) :
			base_type(name_, interface_) {
	}

	explicit xb_sc_fifo_in(in_port_type& parent_) :
			base_type(parent_) {
	}

	xb_sc_fifo_in(const char* name_, in_port_type& parent_) :
			base_type(name_, parent_) {
	}

	xb_sc_fifo_in(this_type& parent_) :
			base_type(parent_) {
	}

	xb_sc_fifo_in(const char* name_, this_type& parent_) :
			base_type(name_, parent_) {
	}

	// destructor (does nothing)

	virtual ~xb_sc_fifo_in() {
	}

	// interface access shortcut methods

	// blocking read

	void read(data_type& value_) {
		(*this)->read(value_);
	}

	data_type read() {
		return (*this)->read();
	}

	// non-blocking read

	bool nb_read(data_type& value_) {
		return (*this)->nb_read(value_);
	}

	bool nb_read_(data_type& value_) {
		return (*this)->nb_read_(value_);
	}

	// get the number of available samples

	int num_available() const {
		return (*this)->num_available();
	}

	// get the data written event

	const sc_event& data_written_event() const {
		return (*this)->data_written_event();
	}

	// use for static sensitivity to data written event

	sc_event_finder& data_written() const {
		return *new sc_event_finder_t<in_if_type>(*this,
				&in_if_type::data_written_event);
	}

	virtual const char* kind() const {
		return "xb_sc_fifo_in";
	}

private:

	// disabled
	xb_sc_fifo_in(const this_type&);
	this_type& operator =(const this_type&);
};

// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII

// ----------------------------------------------------------------------------
//  CLASS : xb_sc_fifo_out<T>
//
//  The sc_fifo<T> output port class.
// ----------------------------------------------------------------------------

template<class T>
class xb_sc_fifo_out: public sc_port<xb_sc_fifo_out_if<T>, 0, SC_ONE_OR_MORE_BOUND> {
public:

	// typedefs

	typedef T data_type;

	typedef xb_sc_fifo_out_if<data_type> if_type;
	typedef sc_port<if_type, 0, SC_ONE_OR_MORE_BOUND> base_type;
	typedef xb_sc_fifo_out<data_type> this_type;

	typedef if_type out_if_type;
	typedef sc_port_b<out_if_type> out_port_type;

public:
	// constructors

	xb_sc_fifo_out() :
			base_type() {
	}

	explicit xb_sc_fifo_out(const char* name_) :
			base_type(name_) {
	}

	explicit xb_sc_fifo_out(out_if_type& interface_) :
			base_type(interface_) {
	}

	xb_sc_fifo_out(const char* name_, out_if_type& interface_) :
			base_type(name_, interface_) {
	}

	explicit xb_sc_fifo_out(out_port_type& parent_) :
			base_type(parent_) {
	}

	xb_sc_fifo_out(const char* name_, out_port_type& parent_) :
			base_type(name_, parent_) {
	}

	xb_sc_fifo_out(this_type& parent_) :
			base_type(parent_) {
	}

	xb_sc_fifo_out(const char* name_, this_type& parent_) :
			base_type(name_, parent_) {
	}

	// destructor (does nothing)

	virtual ~xb_sc_fifo_out() {
	}

	// interface access shortcut methods

	// blocking write

	void write(const data_type& value_) {
		(*this)->write(value_);
	}

	// non-blocking write

	bool nb_write(const data_type& value_) {
		return (*this)->nb_write(value_);
	}

	// get the number of free spaces

	int num_free() const {
		return (*this)->num_free();
	}

	// get the data read event

	const sc_event& data_read_event() const {
		return (*this)->data_read_event();
	}

	// use for static sensitivity to data read event

	sc_event_finder& data_read() const {
		return *new sc_event_finder_t<out_if_type>(*this,
				&out_if_type::data_read_event);
	}

	virtual const char* kind() const {
		return "xb_sc_fifo_out";
	}

private:

	// disabled
	xb_sc_fifo_out(const this_type&);
	this_type& operator =(const this_type&);
};

}
}

#endif /* XBAR_SC_FIFO_PORTS_H_ */
