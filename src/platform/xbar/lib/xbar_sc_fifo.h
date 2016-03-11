/*
 * xb_sc_fifo.h
 *
 *  Created on: 7 Dec 2015
 *      Author: effiong
 */

#ifndef XBAR_SC_FIFO_H_
#define XBAR_SC_FIFO_H_

#include "sysc/communication/sc_communication_ids.h"
#include "sysc/communication/sc_prim_channel.h"
#include "sysc/kernel/sc_event.h"
#include "sysc/kernel/sc_simcontext.h"
#include "sysc/tracing/sc_trace.h"
#include <typeinfo>
#include "xbar_sc_fifo_ifs.h"

namespace dreamcloud {
namespace platform_sclib {
// ----------------------------------------------------------------------------
//  CLASS : xb_sc_fifo<T>
//
//  The xb_sc_fifo<T> primitive channel class.
// ----------------------------------------------------------------------------

template<class T>
class xb_sc_fifo: public xb_sc_fifo_in_if<T>,
		public xb_sc_fifo_out_if<T>,
		public sc_prim_channel {
public:

	// constructors

	explicit xb_sc_fifo(int size_ = 16) :
			sc_prim_channel(sc_gen_unique_name("fifo")), m_data_read_event(
					(std::string(SC_KERNEL_EVENT_PREFIX) + "_read_event").c_str()), m_data_written_event(
					(std::string(SC_KERNEL_EVENT_PREFIX) + "_write_event").c_str()) {
		init(size_);
	}

	explicit xb_sc_fifo(const char* name_, int size_ = 16) :
			sc_prim_channel(name_), m_data_read_event(
					(std::string(SC_KERNEL_EVENT_PREFIX) + "_read_event").c_str()), m_data_written_event(
					(std::string(SC_KERNEL_EVENT_PREFIX) + "_write_event").c_str()) {
		init(size_);
	}

	// destructor

	virtual ~xb_sc_fifo() {
		delete[] m_buf;
	}

	// interface methods

	virtual void register_port(sc_port_base&, const char*);

	// blocking read
	virtual void read(T&);
	virtual T read();

	// non-blocking read
	virtual bool nb_read(T&);
	virtual bool nb_read_(T&);

	// get the number of available samples

	virtual int num_available() const {
		return (m_num_readable - m_num_read);
	}

	// get the data written event

	virtual const sc_event& data_written_event() const {
		return m_data_written_event;
	}

	// blocking write
	virtual void write(const T&);

	// non-blocking write
	virtual bool nb_write(const T&);

	// get the number of free spaces

	virtual int num_free() const {
		return (m_size - m_num_readable - m_num_written);
	}

	// get the data read event

	virtual const sc_event& data_read_event() const {
		return m_data_read_event;
	}

	// other methods

	operator T() {
		return read();
	}

	xb_sc_fifo<T>& operator =(const T& a) {
		write(a);
		return *this;
	}

	void trace(sc_trace_file* tf) const;

	virtual void print(::std::ostream& = ::std::cout) const;
	virtual void dump(::std::ostream& = ::std::cout) const;

	virtual const char* kind() const {
		return "xb_sc_fifo";
	}

protected:

	virtual void update();

	// support methods

	void init(int);

	void buf_init(int);
	bool buf_write(const T&);
	bool buf_read(T&);

protected:

	int m_size;			// size of the buffer
	T* m_buf;			// the buffer
	int m_free;			// number of free spaces
	int m_ri;			// index of next read
	int m_wi;			// index of next write

	sc_port_base* m_reader;	// used for static design rule checking
	sc_port_base* m_writer;	// used for static design rule checking

	int m_num_readable;		// #samples readable
	int m_num_read;		// #samples read during this delta cycle
	int m_num_written;		// #samples written during this delta cycle

	sc_event m_data_read_event;
	sc_event m_data_written_event;

private:

	// disabled
	xb_sc_fifo(const xb_sc_fifo<T>&);
	xb_sc_fifo& operator =(const xb_sc_fifo<T>&);
};

// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII

template<class T>
inline
void xb_sc_fifo<T>::register_port(sc_port_base& port_,
		const char* if_typename_) {
	std::string nm(if_typename_);
	if (nm == typeid(xb_sc_fifo_in_if<T> ).name()
			|| nm == typeid(xb_sc_fifo_blocking_in_if<T> ).name()) {
		// only one reader can be connected
		if (m_reader != 0) {
			SC_REPORT_ERROR(SC_ID_MORE_THAN_ONE_FIFO_READER_, 0);
		}
		m_reader = &port_;
	} else if (nm == typeid(xb_sc_fifo_out_if<T> ).name()
			|| nm == typeid(xb_sc_fifo_blocking_out_if<T> ).name()) {
		// only one writer can be connected
		if (m_writer != 0) {
			SC_REPORT_ERROR(SC_ID_MORE_THAN_ONE_FIFO_WRITER_, 0);
		}
		m_writer = &port_;
	} else {
		SC_REPORT_ERROR(SC_ID_BIND_IF_TO_PORT_,
				"xb_sc_fifo<T> port not recognized");
	}
}

// blocking read

template<class T>
inline
void xb_sc_fifo<T>::read(T& val_) {
	while (num_available() == 0) {
		sc_core::wait(m_data_written_event);
	}
	m_num_read++;
	buf_read(val_);
	request_update();
}

template<class T>
inline T xb_sc_fifo<T>::read() {
	T tmp;
	read(tmp);
	return tmp;
}

// non-blocking read

template<class T>
inline
bool xb_sc_fifo<T>::nb_read(T& val_) {
	if (num_available() == 0) {
		return false;
	}
	m_num_read++;
	buf_read(val_);
	request_update();
	return true;
}

template<class T>
inline
bool xb_sc_fifo<T>::nb_read_(T& val_) {
	if (num_available() == 0) {
		return false;
	}
	if (m_free == m_size) {
		return false;
	}
	val_ = m_buf[m_ri];
	return true;
}

// blocking write

template<class T>
inline
void xb_sc_fifo<T>::write(const T& val_) {
	while (num_free() == 0) {
		sc_core::wait(m_data_read_event);
	}
	m_num_written++;
	buf_write(val_);
	request_update();
}

// non-blocking write

template<class T>
inline
bool xb_sc_fifo<T>::nb_write(const T& val_) {
	if (num_free() == 0) {
		return false;
	}
	m_num_written++;
	buf_write(val_);
	request_update();
	return true;
}

template<class T>
inline
void xb_sc_fifo<T>::trace(sc_trace_file* tf) const {
#if defined(DEBUG_SYSTEMC)
	char buf[32];
	std::string nm = name();
	for( int i = 0; i < m_size; ++ i ) {
		std::sprintf( buf, "_%d", i );
		sc_trace( tf, m_buf[i], nm + buf );
	}
#endif
}

template<class T>
inline
void xb_sc_fifo<T>::print(::std::ostream& os) const {
	if (m_free != m_size) {
		int i = m_ri;
		do {
			os << m_buf[i] << ::std::endl;
			i = (i + 1) % m_size;
		} while (i != m_wi);
	}
}

template<class T>
inline
void xb_sc_fifo<T>::dump(::std::ostream& os) const {
	os << "name = " << name() << ::std::endl;
	if (m_free != m_size) {
		int i = m_ri;
		int j = 0;
		do {
			os << "value[" << i << "] = " << m_buf[i] << ::std::endl;
			i = (i + 1) % m_size;
			j++;
		} while (i != m_wi);
	}
}

template<class T>
inline
void xb_sc_fifo<T>::update() {
	if (m_num_read > 0) {
		m_data_read_event.notify(SC_ZERO_TIME);
	}

	if (m_num_written > 0) {
		m_data_written_event.notify(SC_ZERO_TIME);
	}

	m_num_readable = m_size - m_free;
	m_num_read = 0;
	m_num_written = 0;
}

// support methods

template<class T>
inline
void xb_sc_fifo<T>::init(int size_) {
	buf_init(size_);

	m_reader = 0;
	m_writer = 0;

	m_num_readable = 0;
	m_num_read = 0;
	m_num_written = 0;
}

template<class T>
inline
void xb_sc_fifo<T>::buf_init(int size_) {
	if (size_ <= 0) {
		SC_REPORT_ERROR(SC_ID_INVALID_FIFO_SIZE_, 0);
	}
	m_size = size_;
	m_buf = new T[m_size];
	m_free = m_size;
	m_ri = 0;
	m_wi = 0;
}

template<class T>
inline
bool xb_sc_fifo<T>::buf_write(const T& val_) {
	if (m_free == 0) {
		return false;
	}
	m_buf[m_wi] = val_;
	m_wi = (m_wi + 1) % m_size;
	m_free--;
	return true;
}

template<class T>
inline
bool xb_sc_fifo<T>::buf_read(T& val_) {
	if (m_free == m_size) {
		return false;
	}
	val_ = m_buf[m_ri];
	m_buf[m_ri] = T(); // clear entry for boost::shared_ptr, et al.
	m_ri = (m_ri + 1) % m_size;
	m_free++;
	return true;
}

// ----------------------------------------------------------------------------

template<class T>
inline ::std::ostream&
operator <<(::std::ostream& os, const xb_sc_fifo<T>& a) {
	a.print(os);
	return os;
}

}
}

#endif /* XBAR_SC_FIFO_H_ */
