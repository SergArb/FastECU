#include "checksum_tcu_mitsu_mh8104_can.h"

ChecksumTcuMitsuMH8104Can::ChecksumTcuMitsuMH8104Can()
{

}

ChecksumTcuMitsuMH8104Can::~ChecksumTcuMitsuMH8104Can()
{

}

QByteArray ChecksumTcuMitsuMH8104Can::calculate_checksum(QByteArray romData)
{
    /****************************
     *
     *  FUN_00001578: Check that 0x8000 = 0x5aa5 & 0x7fffe = 0xa55a
     *  FUN_0000288c; Check that
     *
     *
     *
     * *************************/

    uint32_t checksum_balance_value = 0;
    uint32_t checksum_balance_value_address = 0x81fc;
    uint32_t checksum_target = 0x5aa45aab;

    QByteArray msg;
    uint32_t checksum = 0;

    for (int i = 0x8000; i < 0x80000; i += 4)
    {
        checksum += (romData.at(i) << 24) + (romData.at(i + 1) << 16) + (romData.at(i + 2) << 8) + romData.at(i + 3);
    }
    checksum -= 0xffff;
    for (int j = 0; j < 5; j++)
    {
        checksum -= 0xffffffff;
    }
    msg.clear();
    msg.append(QString("Checksum calculated: 0x%1").arg(checksum,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    if (checksum != checksum_target)
    {
        qDebug() << "Checksum value mismatch!";

        QByteArray balance_value_array;

        checksum_balance_value = (uint32_t)(romData.at(checksum_balance_value_address) << 24);
        checksum_balance_value += (uint32_t)(romData.at(checksum_balance_value_address + 1) << 16);
        checksum_balance_value += (uint32_t)(romData.at(checksum_balance_value_address + 2) << 8);
        checksum_balance_value += (uint32_t)(romData.at(checksum_balance_value_address + 3));

        msg.clear();
        msg.append(QString("Balance value before: 0x%1").arg(checksum_balance_value,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        if (checksum > checksum_target)
        {
            checksum_balance_value += checksum_target - checksum;
        }
        else
        {
            checksum_balance_value += checksum_target - checksum;
        }
        msg.clear();
        msg.append(QString("Balance value after: 0x%1").arg(checksum_balance_value,8,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value_array.append((uint8_t)((checksum_balance_value >> 24) & 0xff));
        balance_value_array.append((uint8_t)((checksum_balance_value >> 16) & 0xff));
        balance_value_array.append((uint8_t)((checksum_balance_value >> 8) & 0xff));
        balance_value_array.append((uint8_t)(checksum_balance_value & 0xff));
        romData.replace(checksum_balance_value_address, balance_value_array.length(), balance_value_array);

        qDebug() << "Subaru Mitsu MH8104 CAN CVT TCU checksum corrected";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum corrected");
    }
    else
    {
        qDebug() << "Subaru Mitsu MH8104 CAN CVT TCU checksum OK";
        //QMessageBox::information(this, tr("Subaru Hitachi M32R K-Line/CAN ECU Checksum"), "Checksum OK");
    }

    return romData;
}