#include <string>
#include "ResFileStream.h"

CResFileStream::CResFileStream(IReadResFile *pFile)
{
	m_pReadFile = pFile;
	m_readError = false;

	if (m_pReadFile)
		m_pReadFile->grab();
}

CResFileStream::~CResFileStream()
{
	if (m_pReadFile)
	{
		m_pReadFile->drop();
		m_pReadFile = NULL;
	}

	m_readError = false;
}

int CResFileStream::read(unsigned char *data, int len)
{
	if (m_pReadFile)
	{
		std::cout << "READING" << std::endl;
		int num = m_pReadFile->read(data, (unsigned int)len);
		if (num == 0)
			m_readError = true;

		return num;
	}

	return 0;
}

void CResFileStream::seek(int offset)
{
	if (m_pReadFile)
	{
		bool b = m_pReadFile->seek((long)offset);
		if (b)
			return;
	}
}

int CResFileStream::size() const
{
	if (m_pReadFile)
		return m_pReadFile->getSize();

	return 0;
}

int CResFileStream::tell() const
{
	if (m_pReadFile)
		return m_pReadFile->getPos();

	return 0;
}

bool CResFileStream::canRead() const
{
	if (m_pReadFile)
		return true;

	return false;
}
bool CResFileStream::canWrite() const
{
	if (m_pReadFile)
		return false;

	return false;
}

void CResFileStream::setInputFileObject(IReadResFile *pFile)
{
	m_pReadFile = pFile;
	m_pReadFile->grab();
}
