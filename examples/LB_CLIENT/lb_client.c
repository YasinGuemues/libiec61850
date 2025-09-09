/*
 * lb_client.c
 * 
 */

#include "iec61850_client.h"

#include <stdlib.h>
#include <stdio.h>

#include "hal_thread.h"

void
reportCallbackFunction(void* parameter, ClientReport report)
{
    MmsValue* dataSetValues = ClientReport_getDataSetValues(report);

    printf("received report for %s\n", ClientReport_getRcbReference(report));

    int i;
    for (i = 0; i < 4; i++) {
        ReasonForInclusion reason = ClientReport_getReasonForInclusion(report, i);

        if (reason != IEC61850_REASON_NOT_INCLUDED) {
            printf("  GGIO1.SPCSO%i.stVal: %i (included for reason %i)\n", i,
                    MmsValue_getBoolean(MmsValue_getElement(dataSetValues, i)), reason);
        }
    }
}

int main(int argc, char** argv) {

    char* hostname;
    int tcpPort = 102;

    if (argc > 1)
        hostname = argv[1];
    else
        hostname = "localhost";

    if (argc > 2)
        tcpPort = atoi(argv[2]);

    IedClientError error;

    IedConnection con = IedConnection_create();

    IedConnection_connect(con, &error, hostname, tcpPort);
    printf("Connecting to %s:%i\n", hostname, tcpPort);

    if (error == IED_ERROR_OK)
    {
        printf("Connected\n");

        /* Simply read a float value */
        MmsValue* value = IedConnection_readObject(con, &error, "simpleIOGenericIO/GGIO1.AnIn1.mag.f", IEC61850_FC_MX);

        if (value != NULL)
        {
            if (MmsValue_getType(value) == MMS_FLOAT) {
                float fval = MmsValue_toFloat(value);
                printf("read float value: %f\n", fval);
            }
            else if (MmsValue_getType(value) == MMS_DATA_ACCESS_ERROR) {
                printf("Failed to read value (error code: %i)\n", MmsValue_getDataAccessError(value));
            }

            MmsValue_delete(value);
        }

        /* Read and String, set to another value and read again */
        /* 1. Read */
        MmsValue* char_value = IedConnection_readObject(con, &error, "simpleIOGenericIO/GGIO1.NamPlt.vendor", IEC61850_FC_DC);

        if (MmsValue_getType(char_value) == MMS_VISIBLE_STRING) {
            const char* nice_char = MmsValue_toString(char_value);
            printf("Reading simpleIOGenericIO/GGIO1.NamPlt.vendor object: %s\n", nice_char);
        } else {
            printf("Our nice char value was not read\n");
        }

        /* 2. Write */
        printf("Setting simpleIOGenericIO/GGIO1.NamPlt.vendor object to lemonbeat over MMS write service ...\n");
        value = MmsValue_newVisibleString("lemonbeat");
        IedConnection_writeObject(con, &error, "simpleIOGenericIO/GGIO1.NamPlt.vendor", IEC61850_FC_DC, value);
        if (error != IED_ERROR_OK)
            printf("failed to write simpleIOGenericIO/GGIO1.NamPlt.vendor! (error code: %i)\n", error);

        MmsValue_delete(value);
        Thread_sleep(200);


        /* 3. Read */
        char_value = IedConnection_readObject(con, &error, "simpleIOGenericIO/GGIO1.NamPlt.vendor", IEC61850_FC_DC);

        if (MmsValue_getType(char_value) == MMS_VISIBLE_STRING) {
            const char* nice_char = MmsValue_toString(char_value);
            printf("Reading simpleIOGenericIO/GGIO1.NamPlt.vendor object: %s\n", nice_char);
        } else {
            printf("Our nice char value was not read\n");
        }
        MmsValue_delete(char_value);

        /* Use the MMS control service to set the relay */
        ControlObjectClient control
            = ControlObjectClient_create("simpleIOGenericIO/GGIO1.SPCSO2", con);

        if (control)
        {
            MmsValue* ctlVal = MmsValue_newBoolean(true);
            MmsValue* stVal;

            ControlObjectClient_setOrigin(control, NULL, 3);

            if (ControlObjectClient_operate(control, ctlVal, 0 /* operate now */)) {
                printf("simpleIOGenericIO/GGIO1.SPCSO2 operated successfully\n");
            }
            else {
                printf("failed to operate simpleIOGenericIO/GGIO1.SPCSO2\n");
            }

            MmsValue_delete(ctlVal);

            ControlObjectClient_destroy(control);

            /* Check if status value has changed */

            stVal = IedConnection_readObject(con, &error, "simpleIOGenericIO/GGIO1.SPCSO2.stVal", IEC61850_FC_ST);

            if (error == IED_ERROR_OK) {
                bool state = MmsValue_getBoolean(stVal);
                MmsValue_delete(stVal);

                printf("New status of simpleIOGenericIO/GGIO1.SPCSO2.stVal: %i\n", state);
            }
            else {
                printf("Reading status for simpleIOGenericIO/GGIO1.SPCSO2 failed!\n");
            }

        }
        else {
            printf("Control object simpleIOGenericIO/GGIO1.SPCSO2 not found in server\n");
        }

        /* read data set */
        ClientDataSet clientDataSet = IedConnection_readDataSetValues(con, &error, "simpleIOGenericIO/LLN0.ControlEvents", NULL);

        if (clientDataSet == NULL) {
            printf("failed to read dataset\n");
            goto close_connection;
        }

        /* Read Report Control Block */
        ClientReportControlBlock rcb =
                IedConnection_getRCBValues(con, &error, "simpleIOGenericIO/LLN0.RP.ControlEventsRCB01", NULL);

        if (rcb) {
            /* Install handler for reports (match the RCB we enabled) */
            IedConnection_installReportHandler(con, "simpleIOGenericIO/LLN0.RP.ControlEventsRCB01",
                    ClientReportControlBlock_getRptId(rcb), reportCallbackFunction, NULL);

            /* Set trigger options and enable report
             * Include DATA_CHANGED to get change-triggered reports since stVal has TRG_OPT_DATA_CHANGED
             */
            ClientReportControlBlock_setTrgOps(rcb, TRG_OPT_DATA_CHANGED | TRG_OPT_INTEGRITY | TRG_OPT_GI); // update on data change, periodic updates, updates on request
            ClientReportControlBlock_setRptEna(rcb, true); // activate reporting
            ClientReportControlBlock_setIntgPd(rcb, 5000); // Set reporting period to 5s
            IedConnection_setRCBValues(con, &error, rcb, RCB_ELEMENT_RPT_ENA | RCB_ELEMENT_TRG_OPS | RCB_ELEMENT_INTG_PD, true); // send settings to IED

            if (error != IED_ERROR_OK)
                printf("report activation failed (code: %i)\n", error);

            Thread_sleep(1000);

            /* trigger GI report */
            ClientReportControlBlock_setGI(rcb, true);
            IedConnection_setRCBValues(con, &error, rcb, RCB_ELEMENT_GI, true);

            if (error != IED_ERROR_OK)
                printf("Error triggering a GI report (code: %i)\n", error);

            Thread_sleep(60000);

            /* disable reporting */
            ClientReportControlBlock_setRptEna(rcb, false);
            IedConnection_setRCBValues(con, &error, rcb, RCB_ELEMENT_RPT_ENA, true);

            if (error != IED_ERROR_OK)
                printf("disable reporting failed (code: %i)\n", error);

            ClientDataSet_destroy(clientDataSet);

            ClientReportControlBlock_destroy(rcb);

        } else {
            printf("Report Control Block not found\n");
        }

close_connection:

        IedConnection_close(con);
    }
    else {
        printf("Failed to connect to %s:%i (IED error: %d)\n", hostname, tcpPort, error);
        Thread_sleep(60000);
    }

    IedConnection_destroy(con);

    return 0;
}
