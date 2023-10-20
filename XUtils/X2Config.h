/*
*  Copyright (c) 2022 The X2RTC project authors. All Rights Reserved.
*
*  Please visit https://www.x2rtc.com for detail.
*
*  Author: Eric(eric@x2rtc.com)
*  Twitter: @X2rtc_cloud
*
* The GNU General Public License is a free, copyleft license for
* software and other kinds of works.
*
* The licenses for most software and other practical works are designed
* to take away your freedom to share and change the works.  By contrast,
* the GNU General Public License is intended to guarantee your freedom to
* share and change all versions of a program--to make sure it remains free
* software for all its users.  We, the Free Software Foundation, use the
* GNU General Public License for most of our software; it applies also to
* any other work released this way by its authors.  You can apply it to
* your programs, too.
* See the GNU LICENSE file for more info.
*/
#ifndef __X2_CONFIG_H__
#define __X2_CONFIG_H__

#include <map>
#include <string>

#define INVALID_INTEGER	0
#define INVALID_DOUBLE	0.0
#define NULL_STRING		""

class X2ConfigSet
{
private:
	X2ConfigSet( const X2ConfigSet & c );
	X2ConfigSet & operator = ( const X2ConfigSet & c );
		
public:
	X2ConfigSet();
	~X2ConfigSet();

	void Clear();

	int LoadFromFile( const char * filename, int * errline = NULL );
	int StoreToFile( const char * filename );
	int Dump( void );

public:
	// basic funxtion
	// get value, return NULL when not found.
	// const char * GetValue( const char * sectname, const char * key ) const;
	const char * GetValue(const char * sectname, const char * key, const char * default_val = NULL_STRING) const;

	// set value, return 0 when success;  < 0 when error, 
	// if section key exist, then change it's value, otherwise add a new one
	int SetValue( const char * sectname, const char * key, const char * value );

	// some help function, 
	// get int (double), ATTENTION!! we return 0 (0.0) when error. use GetValue() to test exist or not
	// FIXME : how to deal with default value ??
	// int32_t GetIntVal( const char * sectname, const char * key ) const;
	int GetIntVal( const char * sectname, const char * key, int default_val = INVALID_INTEGER ) const;
	int SetIntVal( const char * sectname, const char * key, int value );

	// double GetDblVal( const char * sectname, const char * key ) const;
	double GetDblVal( const char * sectname, const char * key, double default_val = INVALID_DOUBLE ) const;
	int SetDblVal( const char * sectname, const char * key, double value );

	// int64_t GetInt64Val( const char * sectname, const char * key ) const;
	double GetInt64Val( const char * sectname, const char * key, double default_val = INVALID_INTEGER ) const;
	int SetInt64Val( const char * sectname, const char * key, double value );

	int DelKey( const char * sectname, const char * key );
	int DelSection( const char * sectname );

public:
	// enum all config value, SLOW func, maybe we need an iterator-like interface
	int GetSectionNum() const;
	const char * GetSectionName( int index ) const;
	int GetSectionKeyNum( const char * sectname ) const;
	const char * GetSectionKeyName( const char * sectname, int index ) const;
	// GetFirstSectionName(), GetNextSectionName() etc...
	
protected:
	static char * skip_blank( char * str );
	static char * skip_to_blank( char * str );

protected:

	std::map< std::string, std::map< std::string, std::string > > m_cfg;

	typedef std::map< std::string, std::map< std::string, std::string > >::iterator SectItt;
	typedef std::map< std::string, std::string >::iterator ItemItt;

	typedef std::map< std::string, std::map< std::string, std::string > >::const_iterator const_SectItt;
	typedef std::map< std::string, std::string >::const_iterator const_ItemItt;
	std::string		m_fname;

};

#endif // __X2_CONFIG_H__
