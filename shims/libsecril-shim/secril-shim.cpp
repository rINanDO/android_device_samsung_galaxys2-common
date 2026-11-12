#include <android/api-level.h>
#include "secril-shim.h"
#include "secril-sap.h"

#define ATOI_NULL_HANDLED(x) (x ? atoi(x) : 0)

/* A copy of the original RIL function table. */
static const RIL_RadioFunctions *origRilFunctions;

/* A copy of the ril environment passed to RIL_Init. */
static const struct RIL_Env *rilEnv;

/* Response data for RIL_REQUEST_VOICE_REGISTRATION_STATE */
static const int VOICE_REGSTATE_SIZE = 15 * sizeof(char *);
static char *voiceRegStateResponse[VOICE_REGSTATE_SIZE];

/* Store voice radio technology */
static int voiceRadioTechnology = -1;

/* Store cdma subscription source */
static int cdmaSubscriptionSource = -1;

/* Store sim ruim status */
int simRuimStatus = -1;

/* Store SIM PIN attempts */
int simPinAttempts = 3;

/* Variables and methods for RIL_REQUEST_DEVICE_IDENTITY support */
static char imei[16];
static char imeisv[17];
static bool gotIMEI = false;
static bool gotIMEISV = false;
static bool inIMEIRequest = false;
static bool inIMEISVRequest = false;
static int requestForIMEI = 0;
static int requestForIMEISV = 0;

static bool onRequestSpoofUnsupportedRequest(int request, void *data, size_t datalen, RIL_Token t);
static void onRequestDeviceIdentity(int request, RIL_Token t);


/* Response data for RIL_REQUEST_GET_CELL_INFO_LIST */
static RIL_CellInfo_v12 cellInfoWCDMA;
static RIL_CellInfo_v12 cellInfoGSM;
static RIL_CellInfo_v12 cellInfoList[2];

static RIL_Dial dial;

static void onRequestDial(int request, void *data, RIL_Token t) {
	RIL_UUS_Info uusInfo;

	dial.address = ((RIL_Dial *) data)->address;
	dial.clir = ((RIL_Dial *) data)->clir;
	dial.uusInfo = ((RIL_Dial *) data)->uusInfo;

	if (dial.uusInfo == NULL) {
		uusInfo.uusType = (RIL_UUS_Type) 0;
		uusInfo.uusDcs = (RIL_UUS_DCS) 0;
		uusInfo.uusData = NULL;
		uusInfo.uusLength = 0;
		dial.uusInfo = &uusInfo;
	}

	origRilFunctions->onRequest(request, &dial, sizeof(dial), t);
}


static int
decodeVoiceRadioTechnology (RIL_RadioState radioState) {
    switch (radioState) {
        case RADIO_STATE_SIM_NOT_READY:
        case RADIO_STATE_SIM_LOCKED_OR_ABSENT:
        case RADIO_STATE_SIM_READY:
            return RADIO_TECH_UMTS;

        case RADIO_STATE_RUIM_NOT_READY:
        case RADIO_STATE_RUIM_READY:
        case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:
        case RADIO_STATE_NV_NOT_READY:
        case RADIO_STATE_NV_READY:
            return RADIO_TECH_1xRTT;

        default:
            RLOGD("decodeVoiceRadioTechnology: Invoked with incorrect RadioState");
            return -1;
    }
}

static void OnRequestGetCellInfoList(int request, void *data, size_t datalen, RIL_Token t) {
	RLOGI("%s: got request %s (data:%p datalen:%d)\n", __FUNCTION__,
		requestToString(request),
		data, datalen);

	cellInfoWCDMA.cellInfoType = RIL_CELL_INFO_TYPE_WCDMA;
	cellInfoWCDMA.CellInfo.wcdma.cellIdentityWcdma.mcc = -1;
	cellInfoWCDMA.CellInfo.wcdma.cellIdentityWcdma.mnc = -1;
	cellInfoWCDMA.CellInfo.wcdma.cellIdentityWcdma.psc = -1;

	cellInfoGSM.cellInfoType = RIL_CELL_INFO_TYPE_GSM;
	cellInfoGSM.CellInfo.gsm.cellIdentityGsm.mcc = -1;
	cellInfoGSM.CellInfo.gsm.cellIdentityGsm.mnc = -1;

	if (cellInfoGSM.CellInfo.gsm.cellIdentityGsm.lac > -1 &&
	    cellInfoGSM.CellInfo.gsm.cellIdentityGsm.cid > -1) {
		cellInfoList[0] = cellInfoGSM;
		cellInfoList[1] = cellInfoWCDMA;
		rilEnv->OnRequestComplete(t, RIL_E_SUCCESS, &cellInfoList, sizeof(cellInfoList));
	} else {
		rilEnv->OnRequestComplete(t, RIL_E_SUCCESS, &cellInfoWCDMA, sizeof(cellInfoWCDMA));
	}
}

static void onRequestVoiceRadioTech(int request, void *data, size_t datalen, RIL_Token t) {
	RLOGI("%s: got request %s (data:%p datalen:%d)\n", __FUNCTION__,
		requestToString(request),
		data, datalen);
        RIL_RadioState radioState = origRilFunctions->onStateRequest();

	voiceRadioTechnology = decodeVoiceRadioTechnology(radioState);
	if (voiceRadioTechnology < 0) {
		rilEnv->OnRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
		return;
	}
	rilEnv->OnRequestComplete(t, RIL_E_SUCCESS, &voiceRadioTechnology, sizeof(voiceRadioTechnology));
}

static int
decodeCdmaSubscriptionSource (RIL_RadioState radioState) {
    switch (radioState) {
        case RADIO_STATE_SIM_NOT_READY:
        case RADIO_STATE_SIM_LOCKED_OR_ABSENT:
        case RADIO_STATE_SIM_READY:
        case RADIO_STATE_RUIM_NOT_READY:
        case RADIO_STATE_RUIM_READY:
        case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:
            return CDMA_SUBSCRIPTION_SOURCE_RUIM_SIM;

        case RADIO_STATE_NV_NOT_READY:
        case RADIO_STATE_NV_READY:
            return CDMA_SUBSCRIPTION_SOURCE_NV;

        default:
            RLOGD("decodeCdmaSubscriptionSource: Invoked with incorrect RadioState");
            return -1;
    }
}

static void onRequestCdmaGetSubscriptionSource(int request, void *data, size_t datalen, RIL_Token t) {
	RLOGI("%s: got request %s (data:%p datalen:%d)\n", __FUNCTION__,
		requestToString(request),
		data, datalen);
        RIL_RadioState radioState = (RIL_RadioState)origRilFunctions->onStateRequest();

	cdmaSubscriptionSource = decodeCdmaSubscriptionSource(radioState);
	if (cdmaSubscriptionSource < 0) {
		rilEnv->OnRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
		return;
	}
	rilEnv->OnRequestComplete(t, RIL_E_SUCCESS, &cdmaSubscriptionSource, sizeof(cdmaSubscriptionSource));
}

static void onRequestDeviceIdentity(int request, RIL_Token t) {
	RIL_Errno e = (gotIMEI && gotIMEISV) ? RIL_E_SUCCESS : RIL_E_GENERIC_FAILURE;

	char empty[1] = "";
	char *deviceIdentityResponse[4];
	deviceIdentityResponse[0] = imei;
	deviceIdentityResponse[1] = imeisv;
	deviceIdentityResponse[2] = empty;
	deviceIdentityResponse[3] = empty;

	RLOGD("%s:\t\t\t<<< REQUEST-COMPLETE: %s: (data:%p datalen:%d token:%p error:%d) \n", __FUNCTION__, requestToString(request),
		deviceIdentityResponse,
		sizeof(deviceIdentityResponse),
		t,
		e);

	rilEnv->OnRequestComplete(t, e, deviceIdentityResponse, sizeof(deviceIdentityResponse));
}

static bool onRequestEnterSimPin(int request, void *data, size_t datalen, RIL_Token t) {
	int length = (int)datalen/ sizeof(char *);
	if (length == 2) {
		char **field = (char **) data;
		char *pin = field[0];
		if (pin == NULL) {
			RLOGD("%s: got request %s: Simulating remaining attempts of %d\n", __FUNCTION__, requestToString(request), simPinAttempts);
			rilEnv->OnRequestComplete(t, RIL_E_SUCCESS, &simPinAttempts, sizeof(simPinAttempts));
			return true;
		}
	}
	return false;
}

static bool onRequestSpoofUnsupportedRequest(int request, void *data, size_t datalen, RIL_Token t) {
	bool handled = false;
	RequestInfo *pRI = (RequestInfo *)t;
	if (pRI != NULL && pRI->pCI != NULL) {
		if (!gotIMEI && !inIMEIRequest) {
			// Use this unsupported request to extract IMEI
			inIMEIRequest = true;
			requestForIMEI = request;
			RLOGI("%s: >>> REQUEST\t\t\t: %s (RIL_REQUEST_DEVICE_IDENTITY [1/6]): Using this unsupported request to extract IMEI in preparation for upcoming RIL_REQUEST_DEVICE_IDENTITY\n", __FUNCTION__, requestToString(requestForIMEI));
			pRI->pCI->requestNumber = RIL_REQUEST_GET_IMEI;
			RLOGI("%s: >>> REQUEST\t\t\t: %s (RIL_REQUEST_DEVICE_IDENTITY [2/6])", __FUNCTION__, requestToString(pRI->pCI->requestNumber)); 
			origRilFunctions->onRequest(pRI->pCI->requestNumber, NULL, 0, t);
			handled = true;
		} else if (!gotIMEISV && !inIMEISVRequest) {
			// Use this unsupported request to extract IMEISV
			inIMEISVRequest = true;
			requestForIMEISV = request;
			RLOGI("%s: >>> REQUEST\t\t\t: %s (RIL_REQUEST_DEVICE_IDENTITY [4/6]): Using this unsupported request to extract IMEISV in preparation for upcoming RIL_REQUEST_DEVICE_IDENTITY\n", __FUNCTION__, requestToString(requestForIMEISV));
			pRI->pCI->requestNumber = RIL_REQUEST_GET_IMEISV;
			RLOGI("%s: >>> REQUEST\t\t\t: %s (RIL_REQUEST_DEVICE_IDENTITY [5/6])", __FUNCTION__, requestToString(pRI->pCI->requestNumber)); 
			origRilFunctions->onRequest(pRI->pCI->requestNumber, NULL, 0, t);
			handled = true;
		}
	}
	return handled;
}
static void onRequestUnsupportedRequest(int request, RIL_Token t) {
	RLOGE("%s:\t\t<<< REQUEST-COMPLETE: %s: (token:%p): Request not send to RIL. Sending REQUEST_NOT_SUPPORTED back to libril.\n",
		__FUNCTION__,
		requestToString(request),
		t);
	rilEnv->OnRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
}


static bool is3gpp2(int radioTech) {
    switch (radioTech) {
        case RADIO_TECH_IS95A:
        case RADIO_TECH_IS95B:
        case RADIO_TECH_1xRTT:
        case RADIO_TECH_EVDO_0:
        case RADIO_TECH_EVDO_A:
        case RADIO_TECH_EVDO_B:
        case RADIO_TECH_EHRPD:
            return true;
        default:
            return false;
    }
}

static int
decodeSimStatus (RIL_RadioState radioState) {
   switch (radioState) {
       case RADIO_STATE_SIM_NOT_READY:
       case RADIO_STATE_RUIM_NOT_READY:
       case RADIO_STATE_NV_NOT_READY:
       case RADIO_STATE_NV_READY:
           return -1;
       case RADIO_STATE_SIM_LOCKED_OR_ABSENT:
       case RADIO_STATE_SIM_READY:
       case RADIO_STATE_RUIM_READY:
       case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:
           return radioState;
       default:
           RLOGD("decodeSimStatus: Invoked with incorrect RadioState");
           return -1;
   }
}

static RIL_RadioState
processRadioState(RIL_RadioState newRadioState) {
    if((newRadioState > RADIO_STATE_UNAVAILABLE) && (newRadioState < RADIO_STATE_ON)) {
        int newVoiceRadioTech;
        int newCdmaSubscriptionSource;
        int newSimStatus;

        /* This is old RIL. Decode Subscription source and Voice Radio Technology
           from Radio State and send change notifications if there has been a change */
        newVoiceRadioTech = decodeVoiceRadioTechnology(newRadioState);
        if(newVoiceRadioTech != voiceRadioTechnology) {
            voiceRadioTechnology = newVoiceRadioTech;
            rilEnv->OnUnsolicitedResponse(RIL_UNSOL_VOICE_RADIO_TECH_CHANGED,
                &voiceRadioTechnology, sizeof(voiceRadioTechnology));
        }
        if(is3gpp2(newVoiceRadioTech)) {
            newCdmaSubscriptionSource = decodeCdmaSubscriptionSource(newRadioState);
            if(newCdmaSubscriptionSource != cdmaSubscriptionSource) {
                cdmaSubscriptionSource = newCdmaSubscriptionSource;
                rilEnv->OnUnsolicitedResponse(RIL_UNSOL_CDMA_SUBSCRIPTION_SOURCE_CHANGED,
                        &cdmaSubscriptionSource, sizeof(cdmaSubscriptionSource));
            }
        }
        newSimStatus = decodeSimStatus(newRadioState);
        if(newSimStatus != simRuimStatus) {
            simRuimStatus = newSimStatus;
            rilEnv->OnUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, NULL, 0);
        }

        /* Send RADIO_ON to telephony */
        newRadioState = RADIO_STATE_ON;
    }

    return newRadioState;
}

static bool onRequestGetRadioCapability(RIL_Token t)
{
	RIL_RadioCapability rc[1] =
	{
		{ /* rc[0] */
			RIL_RADIO_CAPABILITY_VERSION, /* version */
			0, /* session */
			RC_PHASE_CONFIGURED, /* phase */
			RAF_GSM | RAF_GPRS | RAF_EDGE | RAF_HSUPA | RAF_HSDPA | RAF_HSPA | RAF_HSPAP | RAF_UMTS, /* rat */
			{ /* logicalModemUuid */
				0,
			},
			RC_STATUS_SUCCESS /* status */
		}
	};
	rilEnv->OnRequestComplete(t, RIL_E_SUCCESS, rc, sizeof(rc));
	return true;
}

static bool onCompleteGetActivityInfo(RIL_Token t)
{
	RIL_ActivityStatsInfo stats[1];
        stats[0].sleep_mode_time_ms = 0;
	stats[0].idle_mode_time_ms = 0;
	for(int i = 0; i < RIL_NUM_TX_POWER_LEVELS; i++) {
		stats[0].tx_mode_time_ms[i] = 0;
	}
        stats[0].rx_mode_time_ms = 0;

	rilEnv->OnRequestComplete(t, RIL_E_SUCCESS, stats, sizeof(stats));
	return true;
}

static RIL_RadioState onStateRequestShim() {
    RIL_RadioState radioState = RADIO_STATE_OFF;
    RIL_RadioState newRadioState = RADIO_STATE_OFF;

    radioState = origRilFunctions->onStateRequest();
    newRadioState = processRadioState(radioState);

    RLOGI("%s: RIL legacy radiostate converted from %d to %d\n", __FUNCTION__, radioState, newRadioState);
    return newRadioState;
}

/*
 * The Samsung RIL only knows how to configure IPv4 data calls. When the APN
 * asks for IPV4V6 the network grants a dual-stack context, the RIL fails with
 * "set_channel_addresses() error: pdp type is not supported" and leaves the
 * PDP context defined, so every retry is then rejected by the modem until a
 * reboot. Always request a plain IPv4 context instead.
 */
static void onRequestSetupDataCall(int request, void *data, size_t datalen, RIL_Token t)
{
	static const size_t PROTOCOL_INDEX = 6;
	static char protocolIp[] = "IP";
	char **params = (char **) data;
	size_t count = datalen / sizeof(char *);

	if (params == NULL || count <= PROTOCOL_INDEX || params[PROTOCOL_INDEX] == NULL ||
			strcmp(params[PROTOCOL_INDEX], "IP") == 0) {
		origRilFunctions->onRequest(request, data, datalen, t);
		return;
	}

	char *newParams[count];
	memcpy(newParams, params, count * sizeof(char *));
	RLOGI("%s: requesting protocol IP instead of %s\n", __FUNCTION__, params[PROTOCOL_INDEX]);
	newParams[PROTOCOL_INDEX] = protocolIp;
	origRilFunctions->onRequest(request, newParams, datalen, t);
}

static void onRequestShim(int request, void *data, size_t datalen, RIL_Token t)
{
	RLOGD("%s:\t\t\t\t\t>>> REQUEST\t\t\t: %s: data:%p datalen:%d token:%p\n", __FUNCTION__, requestToString(request), data, datalen, t);

	switch (request) {
                /* Our RIL doesn't support this, so we implement this ourself */
                case RIL_REQUEST_GET_CELL_INFO_LIST:
			OnRequestGetCellInfoList(request, data, datalen, t);
			return;
                /* Our RIL doesn't support this, so we implement this ourself */
                case RIL_REQUEST_VOICE_RADIO_TECH:
			onRequestVoiceRadioTech(request, data, datalen, t);
			return;
                /* Our RIL doesn't support this, so we implement this ourself */
                case RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE:
			onRequestCdmaGetSubscriptionSource(request, data, datalen, t);
			return;
		/* RIL_REQUEST_GET_IMEI and RIL_REQUEST_GET_IMEISV is deprecated */
		case RIL_REQUEST_DEVICE_IDENTITY:
			onRequestDeviceIdentity(request, t);
			return;
		/* The Samsung RIL crashes if uusInfo is NULL... */
		case RIL_REQUEST_DIAL:
			if (datalen == sizeof(RIL_Dial) && data != NULL) {
				onRequestDial(request, data, t);
				return;
			}
			break;

		case RIL_REQUEST_SETUP_DATA_CALL:
			onRequestSetupDataCall(request, data, datalen, t);
			return;
		/* Necessary; RILJ may fake this for us if we reply not supported, but we can just implement it. */
		case RIL_REQUEST_GET_RADIO_CAPABILITY:
			onRequestGetRadioCapability(t);
			return;
		/* The Samsung RIL doesn't support RIL_REQUEST_SEND_SMS_EXPECT_MORE, reply with RIL_REQUEST_SEND_SMS instead */
		case RIL_REQUEST_SEND_SMS_EXPECT_MORE:
			origRilFunctions->onRequest(RIL_REQUEST_SEND_SMS, data, datalen, t);
			return;
		case RIL_REQUEST_ENTER_SIM_PIN:
			if (!onRequestEnterSimPin(request, data, datalen, t)) {
				origRilFunctions->onRequest(request, data, datalen, t);
				return;
			}
			if (!onRequestSpoofUnsupportedRequest(request, data, datalen, t)) {
				onRequestUnsupportedRequest(request, t);
			}
			return;
		/* The following requests were introduced post-4.3. */
		case RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC:
		case RIL_REQUEST_SIM_OPEN_CHANNEL: /* !!! */
		case RIL_REQUEST_SIM_CLOSE_CHANNEL:
		case RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL:
		case RIL_REQUEST_NV_READ_ITEM:
		case RIL_REQUEST_NV_WRITE_ITEM:
		case RIL_REQUEST_NV_WRITE_CDMA_PRL:
		case RIL_REQUEST_NV_RESET_CONFIG:
		case RIL_REQUEST_SET_UICC_SUBSCRIPTION:
		case RIL_REQUEST_ALLOW_DATA:
		case RIL_REQUEST_GET_HARDWARE_CONFIG:
		case RIL_REQUEST_SIM_AUTHENTICATION:
		case RIL_REQUEST_GET_DC_RT_INFO:
		case RIL_REQUEST_SET_DC_RT_INFO_RATE:
		case RIL_REQUEST_SET_DATA_PROFILE:
		case RIL_REQUEST_SHUTDOWN: /* TODO: Is there something we can do for RIL_REQUEST_SHUTDOWN ? */
		case RIL_REQUEST_SET_RADIO_CAPABILITY:
		case RIL_REQUEST_START_LCE:
		case RIL_REQUEST_STOP_LCE:
		case RIL_REQUEST_PULL_LCEDATA:
			if (!onRequestSpoofUnsupportedRequest(request, data, datalen, t)) {
				onRequestUnsupportedRequest(request, t);
			}
			return;
	}

	origRilFunctions->onRequest(request, data, datalen, t);
}

static void onCompleteRequestGetSimStatus(RIL_Token t, RIL_Errno e, void *response) {
	/* While at it, upgrade the response to RIL_CardStatus_v6 */
	RIL_CardStatus_v5_samsung *p_cur = ((RIL_CardStatus_v5_samsung *) response);
	RIL_CardStatus_v6 v6response;

	v6response.card_state = p_cur->card_state;
	v6response.universal_pin_state = p_cur->universal_pin_state;
	v6response.gsm_umts_subscription_app_index = p_cur->gsm_umts_subscription_app_index;
	v6response.cdma_subscription_app_index = p_cur->cdma_subscription_app_index;
	v6response.ims_subscription_app_index = -1;
	v6response.num_applications = p_cur->num_applications;

	int i;
	for (i = 0; i < RIL_CARD_MAX_APPS; ++i)
		memcpy(&v6response.applications[i], &p_cur->applications[i], sizeof(RIL_AppStatus));

	/* Send the fixed response to libril */
	rilEnv->OnRequestComplete(t, e, &v6response, sizeof(RIL_CardStatus_v6));
}

static void onRequestCompleteVoiceRegistrationState(RIL_Token t, RIL_Errno e, void *response, size_t responselen) {
	char **resp = (char **) response;
        char radioTechUmts = '3';
	memset(voiceRegStateResponse, 0, VOICE_REGSTATE_SIZE);
	for (int index = 0; index < (int)responselen; index++) {
		voiceRegStateResponse[index] = resp[index];
		switch (index) {
			case 1: {
				cellInfoWCDMA.CellInfo.wcdma.cellIdentityWcdma.lac = atoi(voiceRegStateResponse[index]);
				break;
			}
			case 2: {
				cellInfoWCDMA.CellInfo.wcdma.cellIdentityWcdma.cid = atoi(voiceRegStateResponse[index]);
				break;
			}
			case 3:	{
			        // Add RADIO_TECH_UMTS because our RIL doesn't provide this here
				voiceRegStateResponse[index] = &radioTechUmts;
				break;
		        }
			default:
				break;
		}
	}
	rilEnv->OnRequestComplete(t, e, voiceRegStateResponse, VOICE_REGSTATE_SIZE);
}

static void onRequestCompleteDataRegistrationState(RIL_Token t, RIL_Errno e, void *response, size_t responselen) {
	char **resp = (char **) response;
	int length = (int)responselen/ sizeof(char *);

	if (length >= 4) {
		/* Gather Cellidentity data for RIL_REQUEST_GET_CELL_INFO_LIST */
		switch (atoi(resp[3])) {
			case RIL_CELL_INFO_TYPE_GSM: {
				RLOGI("%s: RIL_CELL_INFO_TYPE_GSM: lac:%s cid:%s \n",
					__FUNCTION__,
					resp[1],
					resp[2]);
				cellInfoGSM.CellInfo.gsm.cellIdentityGsm.lac = atoi(resp[1]);
				cellInfoGSM.CellInfo.gsm.cellIdentityGsm.cid = atoi(resp[2]);
				break;
			}
			case RIL_CELL_INFO_TYPE_WCDMA: {
				RLOGI("%s: RIL_CELL_INFO_TYPE_WCDMA: lac:%s cid:%s \n",
					__FUNCTION__,
					resp[1],
					resp[2]);
				cellInfoGSM.CellInfo.gsm.cellIdentityGsm.lac = -1;
				cellInfoGSM.CellInfo.gsm.cellIdentityGsm.cid = -1;
				cellInfoWCDMA.CellInfo.wcdma.cellIdentityWcdma.lac = atoi(resp[1]);
				cellInfoWCDMA.CellInfo.wcdma.cellIdentityWcdma.cid = atoi(resp[2]);
				break;
			}
			default:
				break;
		}
	}
	rilEnv->OnRequestComplete(t, e, response, responselen);
}

static void onRequestCompleteGetImei(RIL_Token t, RIL_Errno e, void *response, size_t responselen) {
	memcpy(&imei, response, responselen);
	RLOGI("%s:\t\t\t<<< REQUEST-COMPLETE: %s (RIL_REQUEST_DEVICE_IDENTITY [3/6]): Got IMEI:%s error:%d\n", __FUNCTION__, requestToString(requestForIMEI), imei, e);
	onRequestUnsupportedRequest(requestForIMEI, t);
	inIMEIRequest = false;
	gotIMEI = true;
}

static void onRequestCompleteGetImeiSv(RIL_Token t, RIL_Errno e, void *response, size_t responselen) {
	memcpy(&imeisv, response, responselen);
	RLOGI("%s:\t\t<<< REQUEST-COMPLETE: %s (RIL_REQUEST_DEVICE_IDENTITY [6/6]): Got IMEISV:%s error:%d\n", __FUNCTION__, requestToString(requestForIMEISV), imeisv, e);
	onRequestUnsupportedRequest(requestForIMEISV, t);
	inIMEISVRequest = false;
	gotIMEISV = true;
}

static void fixupDataCallList(void *response, size_t responselen) {
	RIL_Data_Call_Response_v6 *p_cur = (RIL_Data_Call_Response_v6 *) response;
	int num = responselen / sizeof(RIL_Data_Call_Response_v6);

	int i;
	for (i = 0; i < num; ++i)
		p_cur[i].gateways = p_cur[i].addresses;
}

static void fixupSignalStrength(void *response) {
	int gsmSignalStrength;

	RIL_SignalStrength_v10 *p_cur = ((RIL_SignalStrength_v10 *) response);

	gsmSignalStrength = p_cur->GW_SignalStrength.signalStrength & 0xFF;

	if (gsmSignalStrength < 0 ||
		(gsmSignalStrength > 31 && p_cur->GW_SignalStrength.signalStrength != 99)) {
		gsmSignalStrength = p_cur->CDMA_SignalStrength.dbm;
	}

	/* Fix GSM signal strength */
	p_cur->GW_SignalStrength.signalStrength = gsmSignalStrength;

	/* We don't support LTE - values should be set to INT_MAX */
	p_cur->LTE_SignalStrength.cqi = INT_MAX;
	p_cur->LTE_SignalStrength.rsrp = INT_MAX;
	p_cur->LTE_SignalStrength.rsrq = INT_MAX;
	p_cur->LTE_SignalStrength.rssnr = INT_MAX;
}

static void onRequestCompleteShim(RIL_Token t, RIL_Errno e, void *response, size_t responselen) {
	int request;
	RequestInfo *pRI;

	pRI = (RequestInfo *)t;

	/* If pRI is null, this entire function is useless. */
	if (pRI == NULL)
		goto null_token_exit;

	/* If pCI is null, this entire function is useless. */
	if (pRI->pCI == NULL)
		goto null_token_exit;

	request = pRI->pCI->requestNumber;

	RLOGD("%s:\t\t\t<<< REQUEST-COMPLETE: %s: response:%p responselen:%d token:%p error:%d\n", __FUNCTION__, requestToString(request), response, responselen, t, e);

	switch (request) {
		case RIL_REQUEST_GET_IMEI:
			onRequestCompleteGetImei(t, e, response, responselen);
			return;
		case RIL_REQUEST_GET_IMEISV:
			onRequestCompleteGetImeiSv(t, e, response, responselen);
			return;
                case RIL_REQUEST_VOICE_REGISTRATION_STATE:
                        /* libsecril expects responselen of 60 (bytes) */
                        /* numstrings (15 * sizeof(char *) = 60) */
			if (response != NULL && responselen < VOICE_REGSTATE_SIZE) {
				onRequestCompleteVoiceRegistrationState(t, e, response, responselen);
				return;
			}
			break;
                case RIL_REQUEST_DATA_REGISTRATION_STATE:
			onRequestCompleteDataRegistrationState(t, e, response, responselen);
			return;
		case RIL_REQUEST_GET_SIM_STATUS:
			/* Remove unused extra elements from RIL_AppStatus */
			if (response != NULL && responselen == sizeof(RIL_CardStatus_v5_samsung)) {
				onCompleteRequestGetSimStatus(t, e, response);
				return;
			}
			break;
		case RIL_REQUEST_LAST_CALL_FAIL_CAUSE:
			/* Remove extra element (ignored on pre-M, now crashing the framework) */
			if (responselen > sizeof(int)) {
				rilEnv->OnRequestComplete(t, e, response, sizeof(int));
				return;
			}
			break;
		case RIL_REQUEST_DATA_CALL_LIST:
		case RIL_REQUEST_SETUP_DATA_CALL:
			/* According to the Samsung RIL, the addresses are the gateways?
			 * This fixes mobile data. */
			if (response != NULL && responselen != 0 && (responselen % sizeof(RIL_Data_Call_Response_v6) == 0)) {
				fixupDataCallList(response, responselen);
				rilEnv->OnRequestComplete(t, e, response, responselen);
				return;
			}
			break;
		case RIL_REQUEST_SIGNAL_STRENGTH:
			/* The Samsung RIL reports the signal strength in a strange way... */
			if (response != NULL && responselen >= sizeof(RIL_SignalStrength_v5)) {
				fixupSignalStrength(response);
				rilEnv->OnRequestComplete(t, e, response, responselen);
				return;
			}
			break;
		case RIL_REQUEST_GET_ACTIVITY_INFO:
			onCompleteGetActivityInfo(t);
			return;
	}

null_token_exit:
	rilEnv->OnRequestComplete(t, e, response, responselen);
}

static void onUnsolicitedResponseShim(int unsolResponse, const void *data, size_t datalen)
{
	switch (unsolResponse) {
		case RIL_UNSOL_DATA_CALL_LIST_CHANGED:
			/* According to the Samsung RIL, the addresses are the gateways?
			 * This fixes mobile data. */
			if (data != NULL && datalen != 0 && (datalen % sizeof(RIL_Data_Call_Response_v6) == 0))
				fixupDataCallList((void*) data, datalen);
			break;
		case RIL_UNSOL_SIGNAL_STRENGTH:
			/* The Samsung RIL reports the signal strength in a strange way... */
			if (data != NULL && datalen >= sizeof(RIL_SignalStrength_v5))
				fixupSignalStrength((void*) data);
			break;
	}

	rilEnv->OnUnsolicitedResponse(unsolResponse, data, datalen);
}

static void patchMem(void *libHandle) {
	/*
	 * MAX_TIMEOUT is used for a call to pthread_cond_timedwait_relative_np.
	 * The issue is bionic has switched to using absolute timeouts instead of
	 * relative timeouts, and a maximum time value can cause an overflow in
	 * the function converting relative to absolute timespecs if unpatched.
	 *
	 * By patching this to 0x01FFFFFF from 0x7FFFFFFF, the timeout should
	 * expire in about a year rather than 68 years, and the RIL should be good
	 * up until the year 2036 or so.
	 */
	uint32_t *MAX_TIMEOUT;

	MAX_TIMEOUT = (uint32_t *)dlsym(libHandle, "MAX_TIMEOUT");
	if (CC_UNLIKELY(!MAX_TIMEOUT)) {
		RLOGE("%s: MAX_TIMEOUT could not be found!", __FUNCTION__);
		return;
	}
	RLOGD("%s: MAX_TIMEOUT found at %p!", __FUNCTION__, MAX_TIMEOUT);
	RLOGD("%s: MAX_TIMEOUT is currently 0x%" PRIX32, __FUNCTION__, *MAX_TIMEOUT);
	if (CC_LIKELY(*MAX_TIMEOUT == 0x7FFFFFFF)) {
		*MAX_TIMEOUT = 0x01FFFFFF;
		RLOGI("%s: MAX_TIMEOUT was changed to 0x0%" PRIX32, __FUNCTION__, *MAX_TIMEOUT);
	} else {
		RLOGW("%s: MAX_TIMEOUT was not 0x7FFFFFFF; leaving alone", __FUNCTION__);
	}

}

const RIL_RadioFunctions* RIL_Init(const struct RIL_Env *env, int argc, char **argv)
{
	RIL_RadioFunctions const* (*origRilInit)(const struct RIL_Env *env, int argc, char **argv);
	static RIL_RadioFunctions shimmedFunctions;
	static struct RIL_Env shimmedEnv;
	void *origRil;

	/* Shim the RIL_Env passed to the real RIL, saving a copy of the original */
	rilEnv = env;
	shimmedEnv = *env;
	shimmedEnv.OnRequestComplete = onRequestCompleteShim;
	shimmedEnv.OnUnsolicitedResponse = onUnsolicitedResponseShim;

	/* Open and Init the original RIL. */
	origRil = dlopen(RIL_LIB_PATH, RTLD_GLOBAL);
	if (CC_UNLIKELY(!origRil)) {
		RLOGE("%s: failed to load '" RIL_LIB_PATH  "': %s\n", __FUNCTION__, dlerror());
		return NULL;
	}

	origRilInit = (const RIL_RadioFunctions *(*)(const struct RIL_Env *, int, char **))(dlsym(origRil, "RIL_Init"));
	if (CC_UNLIKELY(!origRilInit)) {
		RLOGE("%s: couldn't find original RIL_Init!\n", __FUNCTION__);
		goto fail_after_dlopen;
	}

	// Fix RIL issues by patching memory
	patchMem(origRil);

	//remove "-c" command line as Samsung's RIL does not understand it - it just barfs instead
	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-c") && i != argc -1) {	//found it
			memcpy(argv + i, argv + i + 2, sizeof(char*[argc - i - 2]));
			argc -= 2;
		}
	}

	origRilFunctions = origRilInit(GetEnv(&shimmedEnv), argc, argv);
	if (CC_UNLIKELY(!origRilFunctions)) {
		RLOGE("%s: the original RIL_Init derped.\n", __FUNCTION__);
		goto fail_after_dlopen;
	}
	SetRadioFunctions(origRilFunctions);

	/* Shim functions as needed. */
	shimmedFunctions = *origRilFunctions;
	shimmedFunctions.onRequest = onRequestShim;
	shimmedFunctions.onStateRequest = onStateRequestShim;

	return &shimmedFunctions;

fail_after_dlopen:
	dlclose(origRil);
	return NULL;
}
