#pragma once

inline std::shared_ptr< boost::asio::io_context > g_ioc
	= std::make_shared<boost::asio::io_context>();