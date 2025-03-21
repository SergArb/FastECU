#include "checksum_ecu_subaru_hitachi_sh72543r.h"

ChecksumEcuSubaruHitachiSh72543r::ChecksumEcuSubaruHitachiSh72543r()
{

}

ChecksumEcuSubaruHitachiSh72543r::~ChecksumEcuSubaruHitachiSh72543r()
{

}

QByteArray ChecksumEcuSubaruHitachiSh72543r::calculate_checksum(QByteArray romData)
{
    /*******************
     *
     * Checksum is calculated between 0x6000 - 0x1fffff, 16bit summation, balance value is word at 0x1ffffe
     * PTR_DAT_000b446c
     *
     ******************/

    QByteArray msg;
    uint16_t chksum = 0;

    for (int i = 0x6000; i < 0x200000; i+=2)
    {
        chksum += ((uint8_t)romData.at(i) << 8) + (uint8_t)romData.at(i + 1);
    }
    msg.clear();
    msg.append(QString("CHKSUM: 0x%1").arg(chksum,4,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    if (chksum != 0x5aa5)
    {
        qDebug() << "Checksum mismatch!";

        QByteArray balance_value_array;
        uint32_t balance_value_array_start = 0x1ffffe;
        uint16_t balance_value = ((uint8_t)romData.at(balance_value_array_start) << 8) + ((uint8_t)romData.at(balance_value_array_start + 1));

        msg.clear();
        msg.append(QString("Balance value before: 0x%1").arg(balance_value,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value += 0x5aa5 - chksum;

        msg.clear();
        msg.append(QString("Balance value after: 0x%1").arg(balance_value,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value_array.append((uint8_t)((balance_value >> 8) & 0xff));
        balance_value_array.append((uint8_t)(balance_value & 0xff));
        romData.replace(balance_value_array_start, balance_value_array.length(), balance_value_array);

        qDebug() << "Checksums corrected";
        QMessageBox::information(nullptr, QObject::tr("Subaru Hitachi SH72543r ECU Checksum"), "Checksums corrected");
    }

    return romData;
}
