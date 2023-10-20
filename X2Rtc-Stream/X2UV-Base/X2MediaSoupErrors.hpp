#ifndef X2_MS_MEDIASOUP_ERRORS_HPP
#define X2_MS_MEDIASOUP_ERRORS_HPP

#include "X2Logger.hpp"
#include <cstdio> // std::snprintf()
#include <stdexcept>

class X2MediaSoupError : public std::runtime_error
{
public:
	explicit X2MediaSoupError(const char* description) : std::runtime_error(description)
	{
	}

public:
	static const size_t bufferSize{ 2000 };
	thread_local static char buffer[];
};

class X2MediaSoupTypeError : public X2MediaSoupError
{
public:
	explicit X2MediaSoupTypeError(const char* description) : X2MediaSoupError(description)
	{
	}
};

// clang-format off
#define MS_THROW_ERROR(desc, ...) \
	do \
	{ \
		MS_ERROR("throwing X2MediaSoupError: " desc, ##__VA_ARGS__); \
		std::snprintf(X2MediaSoupError::buffer, X2MediaSoupError::bufferSize, desc, ##__VA_ARGS__); \
		throw X2MediaSoupError(X2MediaSoupError::buffer); \
	} while (false)

#define MS_THROW_ERROR_STD(desc, ...) \
	do \
	{ \
		MS_ERROR_STD("throwing X2MediaSoupError: " desc, ##__VA_ARGS__); \
		std::snprintf(X2MediaSoupError::buffer, X2MediaSoupError::bufferSize, desc, ##__VA_ARGS__); \
		throw X2MediaSoupError(X2MediaSoupError::buffer); \
	} while (false)

#define MS_THROW_TYPE_ERROR(desc, ...) \
	do \
	{ \
		MS_ERROR("throwing X2MediaSoupTypeError: " desc, ##__VA_ARGS__); \
		std::snprintf(X2MediaSoupError::buffer, X2MediaSoupError::bufferSize, desc, ##__VA_ARGS__); \
		throw X2MediaSoupTypeError(X2MediaSoupError::buffer); \
	} while (false)

#define MS_THROW_TYPE_ERROR_STD(desc, ...) \
	do \
	{ \
		MS_ERROR_STD("throwing X2MediaSoupTypeError: " desc, ##__VA_ARGS__); \
		std::snprintf(X2MediaSoupError::buffer, X2MediaSoupError::bufferSize, desc, ##__VA_ARGS__); \
		throw X2MediaSoupTypeError(X2MediaSoupError::buffer); \
	} while (false)
// clang-format on

#endif	// X2_MS_MEDIASOUP_ERRORS_HPP
