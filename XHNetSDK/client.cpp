#ifdef USE_BOOST
#include <boost/bind.hpp>
#include "client_manager.h"
#include "libnet_error.h"
#include "identifier_generator.h"

client::client(boost::asio::io_context& ioc,
	NETHANDLE srvid,
	read_callback fnread,
	close_callback fnclose,
	bool autoread)
	: m_srvid(srvid)
	, m_id(generate_identifier())
	, m_socket(ioc)
	, m_fnread(fnread)
	, m_fnclose(fnclose)
	, m_fnconnect(NULL)
	, m_closeflag(false)
	, m_timer(ioc)
	, m_autoread(autoread)
	, m_inreading(false)
	, m_usrreadbuffer(NULL)
	, m_onwriting(false)
	, m_currwriteaddr(NULL)
	, m_currwritesize(0)

{
	m_closeflag = false;
	m_readbuff = new uint8_t[CLIENT_MAX_RECV_BUFF_SIZE];
}

client::~client(void)
{
	m_closeflag = true;
	recycle_identifier(m_id);
	if (m_readbuff != NULL)
	{
		delete[] m_readbuff;
		m_readbuff = NULL;
	}
	m_circularbuff.uninit();
}

int32_t client::run()
{
	if (!m_autoread)
	{
		return e_libnet_err_climanualread;
	}

	m_socket.async_read_some(boost::asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
		boost::bind(&client::handle_read,
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));

	return e_libnet_err_noerror;
}

int32_t client::connect(int8_t* remoteip,
	uint16_t remoteport,
	int8_t* localip,
	uint16_t localport,
	bool blocked,
	connect_callback fnconnect,
	uint32_t timeout)
{
	boost::system::error_code err;
	boost::asio::ip::address remoteaddr = boost::asio::ip::address::from_string(reinterpret_cast<char*>(remoteip), err);
	if (err)
	{
		return e_libnet_err_cliinvalidip;
	}

	boost::asio::ip::tcp::endpoint srvep(remoteaddr, remoteport);

	//open socket
	if (!m_socket.is_open())
	{
		m_socket.open(remoteaddr.is_v4() ? boost::asio::ip::tcp::v4() : boost::asio::ip::tcp::v6(), err);
		if (err)
		{
			return e_libnet_err_cliopensock;
		}
	}

	//set callback function
	m_fnconnect = fnconnect;

	//bind local address
	if ((localip && (0 != strcmp(reinterpret_cast<char*>(localip), ""))) || (localport > 0))
	{
		boost::asio::ip::address localaddr;
		if ((localip && (0 != strcmp(reinterpret_cast<char*>(localip), ""))))
		{
			localaddr = boost::asio::ip::address::from_string(reinterpret_cast<char*>(localip), err);
			if (err)
			{
				close();
				return e_libnet_err_cliinvalidip;
			}
		}

		boost::asio::ip::tcp::endpoint localep(localaddr, localport);
		m_socket.bind(localep, err);
		if (err)
		{
			close();
			return e_libnet_err_clibind;
		}
	}

	//set option
	boost::asio::socket_base::reuse_address reuse_address_option(true);
	m_socket.set_option(reuse_address_option, err);
	if (err)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}

	boost::asio::socket_base::send_buffer_size send_buffer_size_option(LISTEN_SEND_BUFF_SIZE);
	m_socket.set_option(send_buffer_size_option, err);
	if (err)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}

	boost::asio::socket_base::receive_buffer_size recv_buffer_size_option(LISTEN_RECV_BUFF_SIZE);
	m_socket.set_option(recv_buffer_size_option, err);
	if (err)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}

	//���ý��գ����ͻ�����
	int  nRecvSize = 1024 * 1024 * 1;
	boost::asio::socket_base::send_buffer_size    SendSize_option(nRecvSize); //���巢�ͻ�������С
	boost::asio::socket_base::receive_buffer_size RecvSize_option(nRecvSize); //������ջ�������С
	m_socket.set_option(SendSize_option); //���÷��ͻ�������С
	m_socket.set_option(RecvSize_option); //���ý��ջ�������С

	//���÷��ͣ����ճ�ʱ
	int  nSendRecvTimer = 5000; //3�볬ʱ
	int nSocket = m_socket.native_handle();
	setsockopt(m_socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&nSendRecvTimer, sizeof(nSendRecvTimer)); //���÷��ͳ�ʱ
	setsockopt(m_socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&nSendRecvTimer, sizeof(nSendRecvTimer)); //���ý��ճ�ʱ

	//���ùرղ�����
	boost::system::error_code ec;
	boost::asio::ip::tcp::no_delay no_delay_option(true);
	m_socket.set_option(no_delay_option, ec);

	//connect timeout
	if (timeout > 0)
	{
		m_timer.expires_from_now(boost::posix_time::milliseconds(timeout));
		m_timer.async_wait(boost::bind(&client::handle_connect_timeout, shared_from_this(), boost::asio::placeholders::error));
	}

	//connect
	if (blocked)
	{
		m_socket.connect(srvep, err);
		m_timer.cancel();
		if (!err)
		{
			run();
			return e_libnet_err_noerror;
		}
		else
		{
			close();
			return e_libnet_err_cliconnect;
		}
	}
	else //sync connect
	{
		m_socket.async_connect(srvep, boost::bind(&client::handle_connect, shared_from_this(), boost::asio::placeholders::error));
		return e_libnet_err_noerror;
	}
}

void client::handle_write(const boost::system::error_code& ec, size_t transize)
{
	if (ec)
	{
		if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
		{
			if (m_fnclose)
			{
				m_fnclose(get_server_id(), get_id());
			}
		}

		return;
	}

	m_circularbuff.read_commit(m_currwritesize);
	m_currwriteaddr = NULL;
	m_currwritesize = 0;

#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_autowrmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_autowrmtx);
#endif
	m_onwriting = write_packet();
}

void client::handle_read(const boost::system::error_code& ec, size_t transize)
{
	if (ec)
	{
		if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
		{
			if (m_fnclose)
			{
				m_fnclose(get_server_id(), get_id());
			}
		}

		return;
	}

	if (m_autoread)
	{
		if (!m_closeflag)
		{
			if (m_fnread)
			{
				m_fnread(get_server_id(), get_id(), m_readbuff, static_cast<uint32_t>(transize), NULL);
			}
			else
				return; //���ٶ�ȡ 
		}
		else
		{
			return; //���ٶ�ȡ 
		}

		if (m_socket.is_open())
		{
			m_socket.async_read_some(boost::asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
				boost::bind(&client::handle_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			m_socket.async_read_some(boost::asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
				boost::bind(&client::handle_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
			printf("close socket\n");
		}
	}
	else
	{
		if (m_fnread)
		{
			m_fnread(get_server_id(), get_id(), m_usrreadbuffer, static_cast<uint32_t>(transize), NULL);
		}

		m_usrreadbuffer = NULL;
		m_inreading = false;
	}
}

void client::handle_connect(const boost::system::error_code& ec)
{
	m_timer.cancel();

	if (ec)
	{
		if (client_manager_singleton::get_mutable_instance().pop_client(get_id()))
		{
			if (m_fnconnect)
			{
				m_fnconnect(get_id(), 0, 0);
			}
		}
	}
	else
	{
		if (m_fnconnect)
		{
			m_fnconnect(get_id(), 1, htons(m_socket.local_endpoint().port()));
		}

		run();
	}
}

int32_t client::read(uint8_t* buffer,
	uint32_t* buffsize,
	bool blocked,
	bool certain)
{
#ifdef LIBNET_MULTI_THREAD_RECV
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_readmtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_readmtx);
#endif
#endif
	if (m_closeflag)
		return e_libnet_err_clisocknotopen;

	if (!buffer || !buffsize || (0 == *buffsize))
	{
		return  e_libnet_err_invalidparam;
	}

	if (m_autoread)
	{
		return e_libnet_err_cliautoread;
	}

	uint32_t readsize = 0;
	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	if (blocked)
	{
		boost::system::error_code err;
		if (certain)
		{
			readsize = static_cast<uint32_t>(boost::asio::read(m_socket, boost::asio::buffer(buffer, *buffsize), err));
			if (err || (0 == readsize))
			{
				*buffsize = 0;
				client_manager_singleton::get_mutable_instance().pop_client(get_id());
				return e_libnet_err_clireaddata;
			}
			else
			{
				return e_libnet_err_noerror;
			}
		}
		else
		{
			readsize = static_cast<uint32_t>(m_socket.read_some(boost::asio::buffer(buffer, *buffsize), err));
			if (err || (0 == readsize))
			{
				*buffsize = 0;
				client_manager_singleton::get_mutable_instance().pop_client(get_id());
				return e_libnet_err_clireaddata;
			}
			else
			{
				*buffsize = readsize;
				return e_libnet_err_noerror;
			}
		}
	}
	else
	{
		if (m_inreading)
		{
			return e_libnet_err_cliprereadnotfinish;
		}

		m_inreading = true;
		m_usrreadbuffer = buffer;

		if (certain)
		{
			boost::asio::async_read(m_socket, boost::asio::buffer(m_usrreadbuffer, *buffsize),
				boost::bind(&client::handle_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			m_socket.async_read_some(boost::asio::buffer(m_usrreadbuffer, *buffsize),
				boost::bind(&client::handle_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}

		return e_libnet_err_noerror;
	}
}

int32_t client::write(uint8_t* data,
	uint32_t datasize,
	bool blocked)
{
#ifdef LIBNET_MULTI_THREAD_SEND
#ifdef LIBNET_USE_CORE_SYNC_MUTEX
	auto_lock::al_lock<auto_lock::al_mutex> al(m_writemtx);
#else
	auto_lock::al_lock<auto_lock::al_spin> al(m_writemtx);
#endif
#endif
	if (m_closeflag)
		return e_libnet_err_clisocknotopen;

	int32_t ret = e_libnet_err_noerror;
	int32_t datasize2 = datasize;

	if (!data || (0 == datasize))
	{
		return e_libnet_err_invalidparam;
	}

	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	if (blocked)
	{
		boost::system::error_code ec;
		unsigned long nSendPos = 0, nSendRet = 0;

		while (datasize2 > 0)
		{//�ĳ�ѭ������
			nSendRet = boost::asio::write(m_socket, boost::asio::buffer(data + nSendPos, datasize2), ec);

			if (!ec)
			{//����û�г���
				if (nSendRet > 0)
				{
					nSendPos += nSendRet;
					datasize2 -= nSendRet;
				}
			}
			else//���ͳ�����������ѭ�����������ѭ��
				break;
		}
		if (!ec)
		{
			return e_libnet_err_noerror;
		}
		else
		{
			client_manager_singleton::get_mutable_instance().pop_client(get_id());
			return e_libnet_err_cliwritedata;
		}
	}
	else
	{
		if (!m_circularbuff.is_init() &&
			!m_circularbuff.init(CLIENT_MAX_SEND_BUFF_SIZE))
		{
			return e_libnet_err_cliinitswritebuff;
		}

		if (datasize != m_circularbuff.write(data, datasize))
		{
			ret = e_libnet_err_cliwritebufffull;
		}

#ifdef LIBNET_USE_CORE_SYNC_MUTEX
		auto_lock::al_lock<auto_lock::al_mutex> al(m_autowrmtx);
#else
		auto_lock::al_lock<auto_lock::al_spin> al(m_autowrmtx);
#endif
		if (!m_onwriting)
		{
			m_onwriting = write_packet();
		}
	}

	return ret;
}

bool client::write_packet()
{
	m_currwriteaddr = m_circularbuff.try_read(CLIENT_PER_SEND_PACKET_SIZE, m_currwritesize);
	if (m_currwriteaddr && (m_currwritesize > 0))
	{
		boost::asio::async_write(m_socket, boost::asio::buffer(m_currwriteaddr, m_currwritesize),
			boost::bind(&client::handle_write,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

		return true;
	}

	return false;
}

void client::close()
{
	std::lock_guard<std::mutex> lock(m_climtx);
	if (!m_closeflag.exchange(true))
	{
		//m_fnconnect = NULL; //ע�͵��������첽��ʽ����ʧ��ʱ��֪ͨ���� 
		m_fnread = NULL;
		//m_timer.cancel();

		if (m_socket.is_open())
		{
			boost::system::error_code ec;
			m_socket.close(ec);
		}
	}
}

void client::handle_connect_timeout(const boost::system::error_code& ec)
{
	if (!ec)
	{
		if (m_socket.is_open())
		{
			boost::system::error_code ec;
			m_socket.close(ec);
		}
	}
}
#else

#include "client_manager.h"
#include "libnet_error.h"
#include "identifier_generator.h"
#include <malloc.h>
#include <iostream>
client::client(asio::io_context& ioc,
	NETHANDLE srvid,
	read_callback fnread,
	close_callback fnclose,
	bool autoread)
	: m_srvid(srvid)
	, m_id(generate_identifier())
	, m_socket(ioc)
	, m_fnread(fnread)
	, m_fnclose(fnclose)
	, m_fnconnect(NULL)
	, m_closeflag(false)
	, m_timer(ioc)
	, m_autoread(autoread)
	, m_inreading(false)
	, m_usrreadbuffer(NULL)
	, m_onwriting(false)
	, m_currwriteaddr(NULL)
	, m_currwritesize(0)
{
	m_closeflag = false;
	m_readbuff = new uint8_t[CLIENT_MAX_RECV_BUFF_SIZE];
}



client::~client(void)
{
	m_closeflag = true;

	recycle_identifier(m_id);
	if (m_readbuff != NULL)
	{
		delete[] m_readbuff;
		m_readbuff = NULL;
	}
	m_circularbuff.uninit();

#ifndef _WIN32
	malloc_trim(0);
#endif
	
}




int32_t client::run()
{
	if (!m_autoread)
	{
		return e_libnet_err_climanualread;
	}

	//m_socket.async_read_some(asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
	//	std::bind(&client::handle_read,
	//		shared_from_this(),
	//		std::placeholders::_1,
	//		std::placeholders::_2));


	auto self(shared_from_this());
	m_socket.async_read_some(asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
		[this, self](std::error_code ec, std::size_t bytes_transferred)
		{
			handle_read(ec, bytes_transferred);
		});

	return e_libnet_err_noerror;
}

int32_t client::connect(int8_t* remoteip,
	uint16_t remoteport,
	int8_t* localip,
	uint16_t localport,
	bool blocked,
	connect_callback fnconnect,
	uint32_t timeout)
{
	std::error_code err;
	asio::ip::address remoteaddr = asio::ip::address::from_string(reinterpret_cast<char*>(remoteip), err);
	if (err)
	{
		return e_libnet_err_cliinvalidip;
	}

	asio::ip::tcp::endpoint srvep(remoteaddr, remoteport);

	//open socket
	if (!m_socket.is_open())
	{
		m_socket.open(remoteaddr.is_v4() ? asio::ip::tcp::v4() : asio::ip::tcp::v6(), err);
		if (err)
		{
			return e_libnet_err_cliopensock;
		}
	}

	//set callback function
	m_fnconnect = fnconnect;

	//bind local address
	if ((localip && (0 != strcmp(reinterpret_cast<char*>(localip), ""))) || (localport > 0))
	{
		asio::ip::address localaddr;
		if ((localip && (0 != strcmp(reinterpret_cast<char*>(localip), ""))))
		{
			localaddr = asio::ip::address::from_string(reinterpret_cast<char*>(localip), err);
			if (err)
			{
				close();
				return e_libnet_err_cliinvalidip;
			}
		}

		asio::ip::tcp::endpoint localep(localaddr, localport);
		m_socket.bind(localep, err);
		if (err)
		{
			close();
			return e_libnet_err_clibind;
		}
	}

	//set option
	asio::socket_base::reuse_address reuse_address_option(true);
	m_socket.set_option(reuse_address_option, err);
	if (err)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}

	asio::socket_base::send_buffer_size send_buffer_size_option(LISTEN_SEND_BUFF_SIZE);
	m_socket.set_option(send_buffer_size_option, err);
	if (err)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}

	asio::socket_base::receive_buffer_size recv_buffer_size_option(LISTEN_RECV_BUFF_SIZE);
	m_socket.set_option(recv_buffer_size_option, err);
	if (err)
	{
		close();
		return e_libnet_err_clisetsockopt;
	}

	//���ý��գ����ͻ�����
	int  nRecvSize = 1024 * 1024 * 1;
	asio::socket_base::send_buffer_size    SendSize_option(nRecvSize); //���巢�ͻ�������С
	asio::socket_base::receive_buffer_size RecvSize_option(nRecvSize); //������ջ�������С
	m_socket.set_option(SendSize_option); //���÷��ͻ�������С
	m_socket.set_option(RecvSize_option); //���ý��ջ�������С

	//���÷��ͣ����ճ�ʱ
	int  nSendRecvTimer = 5000; //3�볬ʱ
	int nSocket = m_socket.native_handle();
	setsockopt(m_socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&nSendRecvTimer, sizeof(nSendRecvTimer)); //���÷��ͳ�ʱ
	setsockopt(m_socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&nSendRecvTimer, sizeof(nSendRecvTimer)); //���ý��ճ�ʱ

	//���ùرղ�����
	std::error_code ec;
	asio::ip::tcp::no_delay no_delay_option(true);
	m_socket.set_option(no_delay_option, ec);

	//connect timeout
	if (timeout > 0)
	{
		m_timer.expires_after(asio::chrono::seconds(timeout));
		m_timer.async_wait(std::bind(&client::handle_connect_timeout, shared_from_this(), std::placeholders::_1));
	}

	//connect
	if (blocked)
	{
		m_socket.connect(srvep, err);
		m_timer.cancel();
		if (!err)
		{
			run();
			return e_libnet_err_noerror;
		}
		else
		{
			close();
			return e_libnet_err_cliconnect;
		}
	}
	else //sync connect
	{
		m_socket.async_connect(srvep, std::bind(&client::handle_connect, shared_from_this(), std::placeholders::_1));
		return e_libnet_err_noerror;
	}
}

void client::handle_write( std::error_code ec, size_t transize)
{
	if (ec)
	{
		if (client_manager::getInstance().pop_client(get_id()))
		{
			if (m_fnclose)
			{
				m_fnclose(get_server_id(), get_id());
			}
		}

		return;
	}

	m_circularbuff.read_commit(m_currwritesize);
	m_currwriteaddr = NULL;
	m_currwritesize = 0;

	std::unique_lock<std::mutex> _lock(m_autowrmtx);
	m_onwriting = write_packet();
}

void client::handle_read(std::error_code ec, size_t transize)
{
	if (ec)
	{
		if (client_manager::getInstance().pop_client(get_id()))
		{
			if (m_fnclose)
			{
				m_fnclose(get_server_id(), get_id());
			}
		}

		return;
	}

	if (m_autoread)
	{
		if (!m_closeflag)
		{
			if (m_fnread)
			{
				m_fnread(get_server_id(), get_id(), m_readbuff, static_cast<uint32_t>(transize), NULL);
			}
			else
				return; //���ٶ�ȡ 
		}
		else
		{
			return; //���ٶ�ȡ 
		}

		if (m_socket.is_open())
		{
	/*		m_socket.async_read_some(boost::asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
						boost::bind(&client::handle_read,
							shared_from_this(),
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred));*/

			auto self(shared_from_this());
			m_socket.async_read_some(asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
				[this, self](std::error_code ec, std::size_t length)
				{
					handle_read(ec, length);				
				});
		}
		else
		{
	/*		m_socket.async_read_some(boost::asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
				boost::bind(&client::handle_read,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));*/

		auto self(shared_from_this());
			m_socket.async_read_some(asio::buffer(m_readbuff, CLIENT_MAX_RECV_BUFF_SIZE),
				[this, self](std::error_code ec, std::size_t length)
				{
					handle_read(ec, length);
				});

			printf("close socket\n");
		}
	}
	else
	{
		if (m_fnread)
		{
			m_fnread(get_server_id(), get_id(), m_usrreadbuffer, static_cast<uint32_t>(transize), NULL);
		}

		m_usrreadbuffer = NULL;
		m_inreading = false;
	}
}

void client::handle_connect(std::error_code ec)
{
	m_timer.cancel();

	if (ec)
	{
		if (client_manager::getInstance().pop_client(get_id()))
		{
			if (m_fnconnect)
			{
				m_fnconnect(get_id(), 0, 0);
			}
		}
	}
	else
	{
		if (m_fnconnect)
		{
			m_fnconnect(get_id(), 1, htons(m_socket.local_endpoint().port()));
		}

		run();
	}
}

int32_t client::read(uint8_t* buffer,
	uint32_t* buffsize,
	bool blocked,
	bool certain)
{
	std::unique_lock<std::mutex> _lock(m_readmtx);
	if (m_closeflag)
		return e_libnet_err_clisocknotopen;
	if (!buffer || !buffsize || (0 == *buffsize))
	{
		return  e_libnet_err_invalidparam;
	}

	if (m_autoread)
	{
		return e_libnet_err_cliautoread;
	}

	uint32_t readsize = 0;
	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	if (blocked)
	{
		std::error_code err;
		if (certain)
		{
			readsize = static_cast<uint32_t>(asio::read(m_socket, asio::buffer(buffer, *buffsize), err));
			if (err || (0 == readsize))
			{
				*buffsize = 0;
				client_manager::getInstance().pop_client(get_id());
				return e_libnet_err_clireaddata;
			}
			else
			{
				return e_libnet_err_noerror;
			}
		}
		else
		{
			readsize = static_cast<uint32_t>(m_socket.read_some(asio::buffer(buffer, *buffsize), err));
			if (err || (0 == readsize))
			{
				*buffsize = 0;
				client_manager::getInstance().pop_client(get_id());
				return e_libnet_err_clireaddata;
			}
			else
			{
				*buffsize = readsize;
				return e_libnet_err_noerror;
			}
		}
	}
	else
	{
		if (m_inreading)
		{
			return e_libnet_err_cliprereadnotfinish;
		}

		m_inreading = true;
		m_usrreadbuffer = buffer;

		if (certain)
		{
			asio::async_read(m_socket,
				asio::buffer(m_usrreadbuffer, *buffsize),
				[this](std::error_code ec, std::size_t length)
				{
					handle_read(ec, length);
				});
		}
		else
		{
			auto self(shared_from_this());
			m_socket.async_read_some(asio::buffer(m_usrreadbuffer, *buffsize),
				[this, self](std::error_code ec, std::size_t length)
				{
					handle_read(ec, length);				
				});
		}

		return e_libnet_err_noerror;
	}
}

int32_t client::write(uint8_t* data,
	uint32_t datasize,
	bool blocked)
{
	std::unique_lock<std::mutex> _lock(m_writemtx);
	if (m_closeflag)
		return e_libnet_err_clisocknotopen;

	int32_t ret = e_libnet_err_noerror;
	int32_t datasize2 = datasize;

	if (!data || (0 == datasize))
	{
		return e_libnet_err_invalidparam;
	}

	if (!m_socket.is_open())
	{
		return e_libnet_err_clisocknotopen;
	}

	if (blocked)
	{
		std::error_code ec;
		unsigned long nSendPos = 0, nSendRet = 0;

		while (datasize2 > 0)
		{//�ĳ�ѭ������
			nSendRet = asio::write(m_socket, asio::buffer(data + nSendPos, datasize2), ec);

			if (!ec)
			{//����û�г���
				if (nSendRet > 0)
				{
					nSendPos += nSendRet;
					datasize2 -= nSendRet;
				}
			}
			else//���ͳ�����������ѭ�����������ѭ��
				break;
		}
		if (!ec)
		{
			return e_libnet_err_noerror;
		}
		else
		{
			client_manager::getInstance().pop_client(get_id());
			return e_libnet_err_cliwritedata;
		}
	}
	else
	{
		if (!m_circularbuff.is_init() &&
			!m_circularbuff.init(CLIENT_MAX_SEND_BUFF_SIZE))
		{
			return e_libnet_err_cliinitswritebuff;
		}

		if (datasize != m_circularbuff.write(data, datasize))
		{
			ret = e_libnet_err_cliwritebufffull;
		}
		std::unique_lock<std::mutex> _lock(m_autowrmtx);

		if (!m_onwriting)
		{
			m_onwriting = write_packet();
		}
	}

	return ret;
}

bool client::write_packet()
{
	m_currwriteaddr = m_circularbuff.try_read(CLIENT_PER_SEND_PACKET_SIZE, m_currwritesize);
	if (m_currwriteaddr && (m_currwritesize > 0))
	{
		auto self(shared_from_this());
		asio::async_write(m_socket, asio::buffer(m_currwriteaddr, m_currwritesize),
			[this, self](std::error_code ec, std::size_t  length)
			{		
					handle_write(ec, length);			
			});
		return true;
	}

	return false;
}

void client::close()
{
	auto_lock::al_lock<auto_lock::al_spin> al(m_climtx);

	if (!m_closeflag.exchange(true))
	{
		//m_fnconnect = NULL; //ע�͵��������첽��ʽ����ʧ��ʱ��֪ͨ���� 
		m_fnread = NULL;
		//m_timer.cancel();

		if (m_socket.is_open())
		{
			std::error_code ec;
			//m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
			m_socket.close(ec);
		}
	}
}

void client::handle_connect_timeout(std::error_code ec1)
{
	if (!ec1)
	{
		if (m_socket.is_open())
		{
			std::error_code ec;
			m_socket.close(ec);
		}
	}
}

#endif

