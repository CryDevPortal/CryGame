/******************************************************************************
** DownloadMgr.cpp
** Mark Tully
** 6/5/10
******************************************************************************/

/*
	allows game data files to be downloaded from a http server via CryNetwork's
	CCryTCPService module

	not to be confused with CDownloadManager in CrySystem, which is windows only
	and doesn't go through the crynetwork abstraction API

	CDownloadManager should probably be replaced with this one
*/

#include "StdAfx.h"
#include "DownloadMgr.h"
#include "StringUtils.h"
#include "Utility/StringUtils.h"
#include "Utility/CryWatch.h"
#include "TypeInfo_impl.h"
#include "GameCVars.h"
#include <IZLibCompressor.h>

static const int		k_localizedResourceSlop=5;								// add a bit of slop to allow localised resources to be added without contributing to fragmentation by causing the vector to realloc. arbitrary number, not critical this is accurate
static const float	k_downloadableResourceHTTPTimeout=8.0f;		// time server has to not respond for before the download is marked as a fail

#if DOWNLOAD_MGR_DBG
static const char		*k_dlm_list="dlm_list";
static const char		*k_dlm_cat="dlm_cat";
static const char		*k_dlm_purge="dlm_purge";

static AUTOENUM_BUILDNAMEARRAY(k_downloadableResourceStateNames,DownloadableResourceStates);

#endif






static const char *k_platformPrefix="pc";


CDownloadableResource::CDownloadableResource() :
	m_port(0),
	m_maxDownloadSize(0),
	m_broadcastedState(k_notBroadcasted),
	m_pService(NULL),
	m_pBuffer(NULL),
	m_bufferUsed(0),
	m_bufferSize(0),
	m_contentLength(0),
	m_contentOffset(0),
	m_state(k_notStarted),
	m_abortDownload(false),
	m_doingHTTPParse(false)
{
	m_listeners.reserve(8);
	m_listenersToRemove.reserve(8);
}

CDownloadableResource::~CDownloadableResource()
{
	if (m_doingHTTPParse)
	{
		ReleaseHTTPParser();
	}
	CRY_ASSERT_MESSAGE((m_state&k_callbackInProgressMask)==0,"Deleting a resource which is being downloaded - CRASH LIKELY!");	// shouldn't happen due to ref counting
	SAFE_DELETE_ARRAY(m_pBuffer);
}

void CDownloadableResource::Reset()
{
	UpdateRemoveListeners();
	stl::free_container(m_listenersToRemove);
}

CDownloadMgr::CDownloadMgr() :
	CGameMechanismBase("DownloadMgr")
{
#if DOWNLOAD_MGR_DBG
	REGISTER_COMMAND(k_dlm_list, (ConsoleCommandFunc)DbgList, 0, "Lists all the resources the download mgr knows about and their state");
	REGISTER_COMMAND(k_dlm_cat, (ConsoleCommandFunc)DbgCat, 0, "Outputs the specified resource as text to the console");
	REGISTER_COMMAND(k_dlm_purge, (ConsoleCommandFunc)DbgPurge, 0, "Removes any downloaded data for the specified resource");
#endif
}

CDownloadMgr::~CDownloadMgr()
{
#if DOWNLOAD_MGR_DBG
	IConsole		*pCon=gEnv->pConsole;
	if (pCon)
	{
		pCon->RemoveCommand(k_dlm_list);
		pCon->RemoveCommand(k_dlm_cat);
		pCon->RemoveCommand(k_dlm_purge);
	}
#endif
}

void CDownloadMgr::Reset()
{
	for (TResourceVector::iterator iter=m_resources.begin(); iter!=m_resources.end(); ++iter)
		(*iter)->Reset();
}

void CDownloadMgr::DispatchCallbacks()
{
	for (TResourceVector::iterator iter=m_resources.begin(); iter!=m_resources.end(); ++iter)
	{
		CDownloadableResourcePtr	pPtr=*iter;

		pPtr->DispatchCallbacks();
	}
}

void CDownloadableResource::BroadcastFail()
{
	m_broadcastedState=CDownloadableResource::k_broadcastedFail;

#if DOWNLOAD_MGR_DBG
	m_downloadFinished=gEnv->pTimer->GetAsyncCurTime();
#endif

	const int numElements = m_listeners.size();
	for(int i=0; i < numElements; i++)
	{
		m_listeners[i]->DataFailedToDownload(this);
	}
}

void CDownloadableResource::BroadcastSuccess()
{
	m_broadcastedState=CDownloadableResource::k_broadcastedSuccess;

#if DOWNLOAD_MGR_DBG
	m_downloadFinished=gEnv->pTimer->GetAsyncCurTime();
#endif

	const int numElements = m_listeners.size();
	for(int i=0; i < numElements; i++)
	{
		m_listeners[i]->DataDownloaded(this);
	}
}

void CDownloadableResource::DispatchCallbacks()
{
	UpdateRemoveListeners();

	if (m_broadcastedState==CDownloadableResource::k_notBroadcasted)
	{
		if (m_state==CDownloadableResource::k_dataAvailable)
		{
			BroadcastSuccess();
		}
		else if (m_state&CDownloadableResource::k_dataPermanentFailMask)
		{
			BroadcastFail();
		}
	}
}

void CDownloadMgr::Update(
	float					inDt)
{
	// perform callback broadcasts from the main thread
	DispatchCallbacks();
}

void CDownloadableResource::UpdateRemoveListeners()
{
	for (CDownloadableResource::TListenerVector::iterator listenerIter(m_listenersToRemove.begin()), end(m_listenersToRemove.end()); listenerIter != end; ++listenerIter)
	{
		stl::find_and_erase(m_listeners, (*listenerIter));
	}

	m_listenersToRemove.clear();
}

void CDownloadableResource::LoadConfig(
	XmlNodeRef				inNode)
{
	XmlString	str;

	inNode->getAttr("port",m_port);
	inNode->getAttr("maxSize",m_maxDownloadSize);

#define ReadXMLStr(key,out)		if (inNode->getAttr(key,str)) { out=str.c_str(); }
	ReadXMLStr("server",m_server);
	ReadXMLStr("name",m_descName);
	ReadXMLStr("prefix",m_urlPrefix);
	if (inNode->getAttr("url",str))
	{
		m_url.Format("%s/%s",k_platformPrefix,str.c_str());
	}
#undef ReadXMLStr
}

bool CDownloadableResource::Validate()
{
	bool		ok=true;

	if (m_port==0 ||
		m_maxDownloadSize==0 ||
		m_server.length()==0 ||
		m_descName.length()==0 ||
		m_url.length()==0)
	{
		ok=false;
	}

	return ok;
}

void CDownloadMgr::Init(
	const char		*pInResourceDescriptionsFile)
{
	m_resources.clear();

	XmlNodeRef		node=GetISystem()->LoadXmlFromFile(pInResourceDescriptionsFile);

	if (!node || strcmp("downloadmgr_config",node->getTag()))
	{
		GameWarning("Failed to load download mgr config file %s", pInResourceDescriptionsFile);
		return;
	}

	CDownloadableResource	defaultResource;
	XmlNodeRef						defaultXML(0);

#ifdef GAME_IS_CRYSIS2
	for (int i=0, n=node->getChildCount(); i<n; i++)
	{
		XmlNodeRef				x=node->getChild(i);
		
		if (x && strcmp(x->getTag(),"default_resource")==0)
		{
			const char			*config=NULL;
			if (x->getAttr("config",&config) && strcmp(config,g_pGameCVars->g_dataCentreConfig)==0)
			{
				defaultXML=x;
				break;
			}
		}
	}
#endif

	if (defaultXML)
	{
		defaultResource.LoadConfig(defaultXML);
	}

	XmlNodeRef				resources=node->findChild("resources");
	if (resources)
	{
		int numResources=resources->getChildCount();

		m_resources.reserve(numResources+k_localizedResourceSlop);

		for (int i=0; i<numResources; ++i)
		{
			bool						ok=false;
			CDownloadableResourcePtr	pParse=new CDownloadableResource(defaultResource);

			if (pParse)
			{
				pParse->LoadConfig(resources->getChild(i));
				ok=pParse->Validate();
			}

			if (!ok)
			{
				GameWarning("Error loading resource description %d %s from %s",i,pParse->m_descName.c_str(),pInResourceDescriptionsFile);
			}
			else
			{
				m_resources.push_back(pParse);
			}
		}
	}
}

// stores received data into more permanent buffer
bool CDownloadableResource::StoreData(
	const char						*pInData,
	size_t							inDataLen)
{
	bool							finishedWithStream=false;
	int								newUsed=0;
	int								newSize=0;
	char*							pNewBuffer=NULL;

	if ( pInData && inDataLen )
	{
		newUsed = m_bufferUsed + inDataLen;

		if ( newUsed > m_bufferSize)
		{
			int						expectedSize=m_contentLength+m_contentOffset;

			if (newUsed<=m_maxDownloadSize)
			{
				// Reallocate buffer
				// don't allocate a buffer smaller than the http header size, and if we parsed the header don't reallocate to smaller than the data size quoted in the content header
				newSize = max(newUsed, max((int)k_httpHeaderSize,expectedSize));
				pNewBuffer = new char[ newSize ];

				if ( m_pBuffer )
				{
					memcpy( pNewBuffer, m_pBuffer, m_bufferUsed );
				}
			}
			else
			{
				m_state=k_failedReplyContentTooLong;
				finishedWithStream=true;
			}
		}
		else
		{
			// Append to old buffer.
			pNewBuffer = m_pBuffer;
		}

		if ( pNewBuffer )
		{
			memcpy( pNewBuffer + m_bufferUsed, pInData, inDataLen );
			m_bufferUsed = newUsed;

			if ( pNewBuffer != m_pBuffer )
			{
				m_bufferSize = newSize;
				SAFE_DELETE_ARRAY( m_pBuffer );
				m_pBuffer = pNewBuffer;
			}
		}
	}

	return finishedWithStream;
}

bool CDownloadableResource::ReceiveHTTPHeader(
	bool							inReceivedEndOfStream)
{
	bool							finishedWithStream=false;

	if (m_state==k_awaitingHTTPResponse)
	{
		// see if we've received the http header yet
		static const char	k_httpOKv10[]="HTTP/1.0 200";
		static const char	k_httpOKv11[]="HTTP/1.1 200";
		static const char	k_httpContentLength[]="Content-Length: ";

		if (m_bufferUsed>=k_httpHeaderSize || inReceivedEndOfStream)
		{
			bool			badHeader=true;

			// check for http/ok response
			if (m_bufferUsed>=(sizeof(k_httpOKv10)-1) && (memcmp(k_httpOKv10,m_pBuffer,sizeof(k_httpOKv10)-1)==0 || memcmp(k_httpOKv11,m_pBuffer,sizeof(k_httpOKv11)-1)==0))
			{
				// check for data payload information
				char		lengthStr[12];
				const char	*contentLength=CryStringUtils::strnstr(m_pBuffer,k_httpContentLength,m_bufferUsed);

				if (contentLength && ((contentLength-m_pBuffer)+sizeof(lengthStr)) < (size_t)m_bufferUsed)
				{
					size_t	found=cry_copyStringUntilFindChar(lengthStr,contentLength+sizeof(k_httpContentLength)-1,sizeof(lengthStr),'\n');

					if (found!=0)
					{
						int	len=atoi(lengthStr);

						if (len>=0 && len<m_maxDownloadSize)
						{
							m_contentLength=len;

							// now scan for end of header, which is denoted by two CRLFs in http protocol
							for (const char *iter=contentLength; iter<(m_pBuffer+m_bufferUsed-3); ++iter)
							{
								if (iter[0]=='\r' && iter[1]=='\n' && iter[2]=='\r' && iter[3]=='\n')			// some bad http servers don't do CRLF, but do LF LF instead. we won't use those servers
								{
									m_contentOffset=int(iter+4-m_pBuffer);
									badHeader=false;
									break;
								}
							}
						}
						else
						{
							m_state=k_failedReplyContentTooLong;
							finishedWithStream=true;
						}
					}
				}
			}

			if (m_state&k_awaitingHTTPResponse)
			{
				if (badHeader)
				{
					m_state=k_failedReplyHasBadHeader;
					finishedWithStream=true;
				}
				else
				{
					m_state=k_awaitingPayload;
				}
			}
		}
	}

	return finishedWithStream;
}

// static
// called when a response is receieved from the http server
// parses http header and receives expected bytes from server
bool CDownloadableResource::ReceiveDataCallback(
	ECryTCPServiceResult	res,
	void*									pArg,
	STCPServiceDataPtr		pUpload,
	const char*						pData,
	size_t								dataLen,
	bool									endOfStream )
{
	bool										finishedWithStream=false;
	CDownloadableResource		*pResource=static_cast<CDownloadableResource*>(pArg);

	if (pResource->m_abortDownload)
	{
		pResource->m_state=k_failedAborted;
		pResource->Release();		// balances AddRef() in StartDownloading()
		finishedWithStream=true;
	}
	else if (res==eCTCPSR_Ok )
	{
		finishedWithStream=pResource->StoreData(pData,dataLen);

		if (!finishedWithStream)
		{
			finishedWithStream=pResource->ReceiveHTTPHeader(endOfStream);
		}

		if (pResource->m_state==k_awaitingPayload && (pResource->m_bufferUsed-pResource->m_contentOffset)>=pResource->m_contentLength)
		{
			pResource->m_state=k_dataAvailable;
		}

		if (pUpload->m_quietTimer>k_downloadableResourceHTTPTimeout)
		{
			pResource->m_state=k_failedReplyTimedOut;
			finishedWithStream=true;
		}

		if (endOfStream || finishedWithStream || pResource->m_state==k_dataAvailable)
		{
			if ((pResource->m_state&(k_dataPermanentFailMask|k_dataAvailable))==0)
			{
				pResource->m_state=k_failedReplyContentTruncated;
			}
			finishedWithStream=true;
			pResource->Release();	// balances AddRef() in StartDownloading()
		}
	}
	else
	{
		pResource->m_state=k_failedServerUnreachable;
		pResource->Release();		// balances AddRef() in StartDownloading()
		finishedWithStream=true;
	}

	return finishedWithStream;
}

CDownloadableResourcePtr CDownloadMgr::FindResourceByName(
	const char			*inResourceName)
{
	CDownloadableResourcePtr	pResult=NULL;

	for (TResourceVector::iterator iter=m_resources.begin(); iter!=m_resources.end(); ++iter)
	{
		CDownloadableResourcePtr		pPtr=*iter;

		if (pPtr->m_descName==inResourceName)
		{
			pResult=pPtr;
			break;
		}
	}

	return pResult;
}

void CDownloadMgr::PurgeLocalizedResourceByName(
	const char									*inResourceName)
{
	CDownloadableResourcePtr		templateResource=FindResourceByName(inResourceName);

	if (templateResource)
	{
		for (TResourceVector::iterator iter=m_resources.begin(); iter!=m_resources.end(); ++iter)
		{
			CDownloadableResourcePtr		pPtr=*iter;

			if (pPtr->m_isLocalisedInstanceOf==templateResource)
			{
				pPtr->CancelDownload();
				m_resources.erase(iter);

				// there should only be one instance per localised template, so we can stop looking now
				break;
			}
		}
	}
}

// finds the localised resource with the name specified
// this will create a localised instance of a resource from a template if it does not yet exist
// if the language has changed since the last time this was called, it will free the previous
// localised instance before creating the new one
CDownloadableResourcePtr CDownloadMgr::FindLocalizedResourceByName(
	const char									*inResourceName)
{
	ScopedSwitchToGlobalHeap		useGlobalHeap;
	CDownloadableResourcePtr		templateResource=FindResourceByName(inResourceName);
	CDownloadableResourcePtr		result=NULL;

	if (templateResource)
	{
		CryFixedStringT<128>			url=templateResource->m_url;
		ICVar											*pLanguage=gEnv->pConsole->GetCVar("g_language");

		CRY_ASSERT_MESSAGE(pLanguage,"can't find language cvar??");
		if (pLanguage)
		{
			url.replace("%LANGUAGE%",pLanguage->GetString());

			for (TResourceVector::iterator iter=m_resources.begin(); iter!=m_resources.end(); ++iter)
			{
				CDownloadableResourcePtr		pPtr=*iter;

				if (pPtr->m_isLocalisedInstanceOf==templateResource)
				{
					if (pPtr->m_url==url)
					{
						result=pPtr;
						break;
					}
					else
					{
						// found an existing localised instance of this template, which isn't for the current language - we should purge this as that language is no longer selected
						pPtr->CancelDownload();
						m_resources.erase(iter);

						// there should only be one instance per localised template, so we can stop looking now
						break;
					}
				}
			}

			if (!result)
			{
				result=new CDownloadableResource();

				if (result)
				{
					result->m_urlPrefix=templateResource->m_urlPrefix;
					result->m_url=url;
					result->m_server=templateResource->m_server;
					result->m_descName.Format("%s_%s",templateResource->m_descName.c_str(),pLanguage->GetString());		// don't want to reuse name of template resource - so make a unique one
					result->m_port=templateResource->m_port;
					result->m_maxDownloadSize=templateResource->m_maxDownloadSize;
					result->m_isLocalisedInstanceOf=templateResource;

					m_resources.push_back(result);
				}
			}
		}
	}

	return result;
}

// counter part of InitHTTPParser() - call before releasing the resource if you have called InitHTTPParser() on it
void CDownloadableResource::ReleaseHTTPParser()
{
	if (m_doingHTTPParse)
	{
		if (m_state&k_callbackInProgressMask)
		{
			// not completed - set to error state
			m_state=k_failedAborted;
			Release();		// balanced against AddRef() in InitHTTPParser()
		}
		m_doingHTTPParse=false;
	}
}

// this is a way of initialising a downloadble resource if you want to use it to parse http replies
void CDownloadableResource::InitHTTPParser()
{
	assert(m_state==k_notStarted);
	assert(!m_doingHTTPParse);
	static const int	k_maxDownloadSizeForHTTPHeader=4*1024;
	m_maxDownloadSize=k_maxDownloadSizeForHTTPHeader;
	m_state=k_awaitingHTTPResponse;
	m_doingHTTPParse=true;
	AddRef();
#if DOWNLOAD_MGR_DBG
	m_downloadStarted=gEnv->pTimer->GetAsyncCurTime();
#endif
}

void CDownloadableResource::StartDownloading()
{
	if (m_state==k_notStarted)
	{
		if (m_port==0)
		{
			GameWarning("Error tried to start downloading on unconfigured CDownloadableResource");
			m_state=k_failedInternalError;
		}
		else
		{
			STCPServiceDataPtr		pTransaction(NULL);

			m_pService=gEnv->pNetwork->GetLobby()->GetLobbyService()->GetTCPService(m_server,m_port);
			if (m_pService)
			{
				static const int					MAX_HEADER_SIZE=300;
				CryFixedStringT<MAX_HEADER_SIZE>	httpHeader;

				// For HTTP 1.0.
				/*httpHeader.Format(
				"GET /%s%s HTTP/1.0\n"
				"\n",
				m_urlPrefix.c_str(),
				m_url.c_str());*/

				// For HTTP 1.1. Needed to download data from servers that are "multi-homed"
				httpHeader.Format(
					"GET /%s%s HTTP/1.1\n"
					"Host: %s:%d\n"
					"\n",
					m_urlPrefix.c_str(),
					m_url.c_str(),
					m_server.c_str(),
					m_port);

				pTransaction=new STCPServiceData();

				if (pTransaction)
				{
					int		len=httpHeader.length();
					pTransaction->length=len;
					pTransaction->pData=new char[len];
					if (pTransaction->pData)
					{
						memcpy(pTransaction->pData,httpHeader.c_str(),len);
						pTransaction->tcpServReplyCb=ReceiveDataCallback;
						pTransaction->pUserArg=this;		// do ref counting manually for callback data
						this->AddRef();
#if DOWNLOAD_MGR_DBG
						m_downloadStarted=gEnv->pTimer->GetAsyncCurTime();
#endif
						m_state=k_awaitingHTTPResponse;
						if (!m_pService->UploadData(pTransaction))
						{
							this->Release();
							pTransaction=NULL;
						}
					}
					else
					{
						pTransaction=NULL;
					}
				}
			}

			if (!pTransaction)
			{
				m_state=k_failedInternalError;
			}
		}
	}
}

bool CDownloadableResource::Purge()
{
	bool		ok=false;

	if (!(m_state&k_callbackInProgressMask))
	{
		m_state=k_notStarted;
		m_broadcastedState=k_notBroadcasted;
		m_abortDownload=false;

		SAFE_DELETE_ARRAY(m_pBuffer);
		m_bufferUsed=m_bufferSize=0;
		m_contentLength=m_contentOffset=0;

		ok=true;
	}

	return ok;
}

void CDownloadableResource::SetDownloadInfo(const char* pUrl, const char* pUrlPrefix, const char* pServer, const int port, const int maxDownloadSize, const char* pDescName/*=NULL*/)
{
	CRY_ASSERT(pUrl && pServer && maxDownloadSize>0);

	CRY_ASSERT_MESSAGE(m_port==0,"You cannot call CDownloadableResource::SetDownloadInfo() twice on the same resource");		// not unless the code is strengthened anyway
	CRY_ASSERT_MESSAGE(port!=0,"You cannot request a download from port 0");		// invalid generally, but also we use 0 as not initialized here

	m_port = port;
	m_maxDownloadSize = maxDownloadSize;

	m_server = pServer;
	m_descName = pDescName ? pDescName : pUrl;
	m_urlPrefix = pUrlPrefix;
	m_url = pUrl;
}

CDownloadableResource::TState CDownloadableResource::GetRawData(
	char				**pOutData,
	int					*pOutLen)
{
	StartDownloading();

	if (m_state==k_dataAvailable)
	{
		*pOutData=m_pBuffer+m_contentOffset;
		*pOutLen=m_contentLength;
	}
	else
	{
		*pOutData=NULL;
		*pOutLen=0;
	}

	return m_state;
}

// func returns true if the data is correctly encrypted and signed. caller is then responsible for calling delete[] on the returned data buffer
// returns false if there is a problem, caller has no data to free if false is returned
bool CDownloadableResource::DecryptAndCheckSigning(
	const char	*pInData,
	int					inDataLen,
	char				**pOutData,
	int					*pOutDataLen,
	const char	*pDecryptionKey,
	int					decryptionKeyLength,
	const char	*pSigningSalt,
	int					signingSaltLength)
{
	char							*pOutput=NULL;
	int								outDataLen=0;
	bool							valid=false;

	if (inDataLen>16)
	{
		INetwork					*pNet=GetISystem()->GetINetwork();
		IZLibCompressor		*pZLib=GetISystem()->GetIZLibCompressor();
		TCipher						cipher=pNet->BeginCipher((uint8*)pDecryptionKey,decryptionKeyLength);
		if (cipher)
		{
			pOutput=new char[inDataLen];
			if (pOutput)
			{
				pNet->Decrypt(cipher,(uint8*)pOutput,(const uint8*)pInData,inDataLen);

				// validate cksum to check for successful decryption and for data signing
				SMD5Context		ctx;
				char					digest[16];

				pZLib->MD5Init(&ctx);
				pZLib->MD5Update(&ctx,pSigningSalt,signingSaltLength);
				pZLib->MD5Update(&ctx,pOutput,inDataLen-16);
				pZLib->MD5Final(&ctx,digest);

				if (memcmp(digest,pOutput+inDataLen-16,16)!=0)
				{
					CryLog("MD5 fail on downloaded data");
				}
				else
				{
					CryLog("data passed decrypt and MD5 checks");
					valid=true;
				}
			}
			pNet->EndCipher(cipher);
		}
	}

	if (valid)
	{
		*pOutData=pOutput;
		*pOutDataLen=inDataLen-16;
	}
	else
	{
		if (pOutput)
		{
			delete [] pOutput;
		}
		*pOutData=NULL;
		*pOutDataLen=0;
	}

	return valid;
}

CDownloadableResource::TState CDownloadableResource::GetDecryptedData(
	char				*pBuffer,
	int					*pOutLen,
	const char	*pDecryptionKey,
	int					decryptionKeyLength,
	const char	*pSigningSalt,
	int					signingSaltLength)
{
	StartDownloading();

	bool success = false;
	if (m_state==k_dataAvailable)
	{
		char* pEncryptedBuffer = m_pBuffer+m_contentOffset;
		int encryptedLength = m_contentLength;
		char* pDecryptedBuffer = NULL;
		int decryptedLength = 0;
		if (DecryptAndCheckSigning(pEncryptedBuffer, encryptedLength, &pDecryptedBuffer, &decryptedLength,
			pDecryptionKey, decryptionKeyLength, pSigningSalt, signingSaltLength))
		{
			unsigned long destSize = *pOutLen;
			int error = gEnv->pCryPak->RawUncompress(pBuffer, &destSize, pDecryptedBuffer, (unsigned long)decryptedLength);

			if (error == 0)
			{
				*pOutLen = (int)destSize;
				success = true;
			}
			else
			{
				CryLog("Failed to uncompress data");
			}

			delete [] pDecryptedBuffer;
			pDecryptedBuffer = NULL;
		}
	}
	if (!success)
	{
		*pOutLen=0;
	}

	return m_state;
}

CDownloadableResource::TState CDownloadableResource::GetProgress(
	int					*outBytesDownloaded,
	int					*outTotalBytesToDownload)
{
	if (m_state&(k_dataAvailable|k_awaitingPayload))
	{
		*outBytesDownloaded=m_bufferUsed-m_contentOffset;
		*outTotalBytesToDownload=m_contentLength;
	}
	else
	{
		*outBytesDownloaded=*outTotalBytesToDownload=0;
	}

	return m_state;
}

void CDownloadableResource::AddDataListener(
	IDataListener	*pInListener)
{
	CRY_ASSERT_MESSAGE(std::find(m_listeners.begin(), m_listeners.end(), pInListener) == m_listeners.end(), "A downloadable resource should not have two instances of the same listener");
	m_listeners.push_back(pInListener);

	switch (m_broadcastedState)
	{
		case k_broadcastedSuccess:
			pInListener->DataDownloaded(this);
			break;

		case k_broadcastedFail:
			pInListener->DataFailedToDownload(this);
			break;

		default:
			break;
	}

	StartDownloading();
}

void CDownloadableResource::RemoveDataListener(
	IDataListener	*pInListener)
{
	// these are nothing to worry about - it is not uncommon to unregister twice if there is an additional unregister in a destructor
	// better do unreg twice than have a dangling ptr
//	CRY_ASSERT_MESSAGE(std::find(m_listenersToRemove.begin(), m_listenersToRemove.end(), pInListener) == m_listenersToRemove.end(), "Trying to remove a data listener twice!");
//	CRY_ASSERT_MESSAGE(std::find(m_listeners.begin(), m_listeners.end(), pInListener) != m_listeners.end(), "trying to remove a listener thats not in the listners list");
	m_listenersToRemove.push_back(pInListener);
}

IDataListener::~IDataListener()
{
#if DOWNLOAD_MGR_DBG
	if (g_pGame)
	{
		CDownloadMgr		*pMgr=g_pGame->GetDownloadMgr();
		if (pMgr)
		{
			CRY_ASSERT_MESSAGE(!pMgr->IsListenerListening(this),"IDataListener is being deleted but it is still registered as a listener - bad code!");
		}
	}
#endif

}

// only call from the main thread as it will broadcast to listeners which are not required to be thread safe
void CDownloadableResource::CancelDownload()
{
	// it is possible to have a callback in flight and for it to have broadcasted, if it has been cancelled previously
	if ((m_state&k_callbackInProgressMask) && (m_broadcastedState==k_notBroadcasted))
	{
		m_abortDownload=true;		// will abort on next callback received from network thread
		UpdateRemoveListeners();
		BroadcastFail();				// broadcast the fail now
	}
}

void CDownloadMgr::WaitForDownloadsToFinish(const char** resources, int numResources, float timeout)
{
	CDownloadableResourcePtr* pResources=new CDownloadableResourcePtr[numResources];
	for (int i=0; i<numResources; ++i)
	{
		CDownloadableResourcePtr pRes = FindResourceByName(resources[i]);
		if (pRes)
		{
			pRes->StartDownloading();
		}
		pResources[i] = pRes;
	}
	CTimeValue startTime = gEnv->pTimer->GetAsyncTime();
	while (true)
	{
		bool allFinished = true;
		for (int i=0; i<numResources; ++i)
		{
			CDownloadableResourcePtr pRes = pResources[i];
			if (pRes)
			{
				CDownloadableResource::TState state = pRes->GetState();
				if (state & CDownloadableResource::k_callbackInProgressMask)
				{
					allFinished = false;
					break;
				}
			}
		}
		CTimeValue currentTime = gEnv->pTimer->GetAsyncTime();
		if (allFinished || currentTime.GetDifferenceInSeconds(startTime) > timeout)
		{
			break;
		}
		CrySleep(100);
	};
	delete [] pResources;
	DispatchCallbacks();
}

#if DOWNLOAD_MGR_DBG

bool CDownloadMgr::IsListenerListening(
	IDataListener		*inListener)
{
	bool						found=false;

	for (TResourceVector::iterator iter=m_resources.begin(); iter!=m_resources.end(); ++iter)
	{
		CDownloadableResourcePtr		pPtr=*iter;

		if (std::find(pPtr->m_listeners.begin(),pPtr->m_listeners.end(),inListener)!=pPtr->m_listeners.end())			// if in listener list
		{
			if (std::find(pPtr->m_listenersToRemove.begin(),pPtr->m_listenersToRemove.end(),inListener)==pPtr->m_listenersToRemove.end())		// and not in remove list
			{
				found=true;
				break;
			}
		}
	}

	return found;
}

// static
string CDownloadableResource::GetStateAsString(
	TState					inState)
{
	return AutoEnum_GetStringFromBitfield(inState,k_downloadableResourceStateNames,ARRAY_COUNT(k_downloadableResourceStateNames));
}

// static
void CDownloadMgr::DbgList(
	IConsoleCmdArgs			*pInArgs)
{
	CDownloadMgr			*pDlm=static_cast<CGame*>(gEnv->pGame)->GetDownloadMgr();
	int						count=0;

	for (TResourceVector::iterator iter=pDlm->m_resources.begin(); iter!=pDlm->m_resources.end(); ++iter)
	{
		CDownloadableResourcePtr	pPtr=*iter;
		const char								*ss=CDownloadableResource::GetStateAsString(pPtr->m_state).c_str();

		float				timeElapsed=0.0f;
		if (!(pPtr->m_state&CDownloadableResource::k_notStarted))
		{
			if (pPtr->m_state&CDownloadableResource::k_dataAvailable)
			{
				timeElapsed=(pPtr->m_downloadFinished-pPtr->m_downloadStarted).GetSeconds();
			}
			else
			{
				timeElapsed=(CTimeValue(gEnv->pTimer->GetAsyncCurTime())-pPtr->m_downloadStarted).GetSeconds();
			}
		}

		CryLogAlways("Resource %d : %s : state %s    : content %d B / %d B (%.2f bytes/sec) (%.2f elapsed)",
			count,pPtr->m_descName.c_str(), ss, pPtr->m_bufferUsed-pPtr->m_contentOffset, pPtr->m_contentLength, pPtr->GetTransferRate(),timeElapsed);

		count++;
	}

	CryLogAlways("%d resources",count);
}

// static
void CDownloadMgr::DbgPurge(
	IConsoleCmdArgs				*pInArgs)
{
	CDownloadMgr				*pDlm=static_cast<CGame*>(gEnv->pGame)->GetDownloadMgr();

	if (pInArgs->GetArgCount()==2)
	{
		string						resName=pInArgs->GetArg(1);
		CDownloadableResourcePtr	pRes=pDlm->FindResourceByName(resName);

		if (!pRes)
		{
			CryLogAlways("Unknown resource '%s'",resName.c_str());
		}
		else
		{
			string		wasState=pRes->GetStateAsString();
			if (pRes->Purge())
			{
				CryLogAlways("Resource '%s' purged, was in state %s",resName.c_str(),wasState.c_str());
			}
			else
			{
				CryLogAlways("Resource '%s' NOT purged, callback in progress, in state %s",resName.c_str(),wasState.c_str());
			}
		}
	}
	else
	{
		CryLogAlways("usage: dlm_purge <resourcename>");
	}
}

// static
void CDownloadMgr::DbgCat(
	IConsoleCmdArgs				*pInArgs)
{
	CDownloadMgr				*pDlm=static_cast<CGame*>(gEnv->pGame)->GetDownloadMgr();

	if (pInArgs->GetArgCount()==2)
	{
		string						resName=pInArgs->GetArg(1);
		CDownloadableResourcePtr	pRes=pDlm->FindResourceByName(resName);

		if (!pRes)
		{
			CryLogAlways("Unknown resource '%s'",resName.c_str());
		}
		else
		{
			char								*pPtr;
			int									size;
			CDownloadableResource::TState		state=pRes->GetRawData(&pPtr,&size);

			if (state==CDownloadableResource::k_dataAvailable)
			{
				char		*pMyBigBuffer=new char[size+1];
				if (pMyBigBuffer)
				{
					memcpy(pMyBigBuffer,pPtr,size);
					pMyBigBuffer[size]=0;
					CryLogAlways("%s",pMyBigBuffer);
					delete [] pMyBigBuffer;
					// ... i don't even want to think about what all the above just did to the memory state...

					CryLogAlways("Resource '%s' %d bytes downloaded",pRes->m_descName.c_str(),size);
				}
				else
				{
					CryLogAlways("Out of mem");
				}
			}
			else
			{
				CryLogAlways("Resource '%s' state %s",pRes->m_descName.c_str(),pRes->GetStateAsString().c_str());
			}
		}
	}
	else
	{
		CryLogAlways("usage: dlm_cat <resourcename>");
	}
}

void CDownloadableResource::DebugWatchContents()
{
		const char *ss=CDownloadableResource::GetStateAsString(m_state).c_str();
		float				timeElapsed=0.0f;
		if (!(m_state&k_notStarted))
		{
			if (m_state&k_dataAvailable)
			{
				timeElapsed=(m_downloadFinished-m_downloadStarted).GetSeconds();
			}
			else
			{
				timeElapsed=(CTimeValue(gEnv->pTimer->GetAsyncCurTime())-m_downloadStarted).GetSeconds();
			}
		}
		CryWatch("Resource %s : state %s    : content %d B / %d B (%.2f bytes/sec) (elapsed %.2f sec)",
						 m_descName.c_str(), ss, m_bufferUsed-m_contentOffset, m_contentLength, GetTransferRate(),timeElapsed);
}

float CDownloadableResource::GetTransferRate()
{
	float		trate=0.0f;

	if (!(m_state&k_notStarted))
	{
		if (m_state&k_dataAvailable)
		{
			float		took=(m_downloadFinished-m_downloadStarted).GetSeconds();

			if (took>0.0f)
			{
				trate=float(m_bufferUsed)/float(took);
			}
		}
		else
		{
			CTimeValue now=gEnv->pTimer->GetAsyncCurTime();
			float			taken=(now-m_downloadStarted).GetSeconds();

			if (taken>0.0f)
			{
				trate=float(m_bufferUsed)/float(taken);
			}
		}
	}

	return trate;
}

#endif
