#include "flash_ecu_subaru_mitsu_m32r_kline.h"

//QT_CHARTS_USE_NAMESPACE

FlashEcuSubaruMitsuM32rKline::FlashEcuSubaruMitsuM32rKline(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EcuOperationsWindow)
    , ecuCalDef(ecuCalDef)
    , cmd_type(cmd_type)
{
    ui->setupUi(this);

    if (cmd_type == "test_write")
        this->setWindowTitle("Test write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "write")
        this->setWindowTitle("Write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "read")
        this->setWindowTitle("Read ROM from TCU");

    this->serial = serial;
}

void FlashEcuSubaruMitsuM32rKline::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

    mcu_type_string = ecuCalDef->McuType;
    mcu_type_index = 0;

    while (flashdevices[mcu_type_index].name != 0)
    {
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }
    QString mcu_name = flashdevices[mcu_type_index].name;
    //send_log_window_message("MCU type: " + mcu_name + " and index: " + mcu_type_index, true, true);
    qDebug() << "MCU type:" << mcu_name << mcu_type_string << "and index:" << mcu_type_index;

    kernel = ecuCalDef->Kernel;
    flash_method = ecuCalDef->FlashMethod;

    if (cmd_type == "read")
    {
        send_log_window_message("Read memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Read memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }
    else if (cmd_type == "test_write")
    {
        test_write = true;
        send_log_window_message("Test write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Test write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }
    else if (cmd_type == "write")
    {
        test_write = false;
        send_log_window_message("Write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }

    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_iso14230_connection(true);
    serial->open_serial_port();
    serial->change_port_speed("4800");
    serial->set_add_iso14230_header(false);
    tester_id = 0xF0;
    target_id = 0x10;

    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            send_log_window_message("Connecting to Subaru ECU Mitsubishi K-Line bootloader, please wait...", true, true);

            result = connect_bootloader_subaru_ecu_mitsu_kline();

            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    send_log_window_message("Reading ROM from ECU Subaru Mitsubishi using K-Line", true, true);
                    result = read_mem_subaru_ecu_mitsu_kline(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    send_log_window_message("Writing ROM to ECU Subaru Mitsubishi using K-Line", true, true);
                    result = write_mem_subaru_ecu_mitsu_kline(test_write);
                }
            }
            emit external_logger("Finished");

            if (result == STATUS_SUCCESS)
            {
                QMessageBox::information(this, tr("ECU Operation"), "ECU operation was succesful, press OK to exit");
                this->close();
            }
            else
            {
                QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, press OK to exit and try again");
            }
            break;
        case QMessageBox::Cancel:
            qDebug() << "Operation canceled";
            this->close();
            break;
        default:
            QMessageBox::warning(this, tr("Connecting to ECU"), "Unknown operation selected!");
            qDebug() << "Unknown operation selected!";
            this->close();
            break;
    }
}

FlashEcuSubaruMitsuM32rKline::~FlashEcuSubaruMitsuM32rKline()
{
    delete ui;
}

void FlashEcuSubaruMitsuM32rKline::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

/*
 * Connect to Subaru TCU Mitsubishi CAN bootloader
 *
 * @return success
 */
int FlashEcuSubaruMitsuM32rKline::connect_bootloader_subaru_ecu_mitsu_kline()
{
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;
    QByteArray output;

    QString msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    delay(100);

    send_log_window_message("Initializing k-line communications", true, true);

    received = send_subaru_mitsu_sid_bf_ssm_init();

    //if (received == "")
    //    return STATUS_ERROR;
    //qDebug() << "SID_BF received:" << parse_message_to_hex(received);
    received.remove(0, 8);
    received.remove(5, received.length() - 5);
    //qDebug() << "Received length:" << received.length();
    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }
    QString calid = msg;
    //QString ecuid = QString("%1%2%3%4%5").arg(received.at(0),2,16,QLatin1Char('0')).toUpper().arg(received.at(1),2,16,QLatin1Char('0')).arg(received.at(2),2,16,QLatin1Char('0')).arg(received.at(3),2,16,QLatin1Char('0')).arg(received.at(4),2,16,QLatin1Char('0'));

    send_log_window_message("Init Success: CAL ID = " + calid, true, true);
    qDebug() << "CAL ID = " << calid;

    // Start communication

    send_log_window_message("Start communication ok", true, true);
    qDebug() << "Start communication ok";

    // Request timing parameters
    received = send_subaru_mitsu_sid_81_start_communication();
//    send_log_window_message("Received = " + parse_message_to_hex(received), true, true);

    received = send_subaru_mitsu_sid_83_request_timings();
//    send_log_window_message("Received = " + parse_message_to_hex(received), true, true);
    //if (received == "" || (uint8_t)received.at(4) != 0xC3)
    //    return STATUS_ERROR;

//    send_log_window_message("Request timing parameters ok", true, true);
//    qDebug() << "Request timing parameters ok";

    // Request seed
    received = send_subaru_mitsu_sid_27_request_seed();
    send_log_window_message("SID_27_01 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
//        return STATUS_ERROR;

    send_log_window_message("Seed request ok", true, true);
//    qDebug() << "Seed request ok";

    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    seed_key = subaru_ecu_mitsu_generate_kline_seed_key(seed);
    msg = parse_message_to_hex(seed_key);
    //qDebug() << "Calculated seed key: " + msg + ", sending to ECU";
    send_log_window_message("Calculated seed key: " + msg + ", sending to ECU", true, true);
    send_log_window_message("Sending seed key", true, true);
//    qDebug() << "Sending seed key";

    received = send_subaru_mitsu_sid_27_send_seed_key(seed_key);
    send_log_window_message("SID_27_02 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
//       return STATUS_ERROR;

    send_log_window_message("Seed key ok", true, true);
//    qDebug() << "Seed key ok";

    // Start diagnostic session
    received = send_subaru_mitsu_sid_10_start_diagnostic();
    send_log_window_message("SID_10 = " + parse_message_to_hex(received), true, true);
//    if (received == "" || (uint8_t)received.at(4) != 0x50)
//        return STATUS_ERROR;

    send_log_window_message("Start diagnostic session ok", true, true);
    //qDebug() << "Start diagnostic session ok";

    return STATUS_SUCCESS;
}


/*
 * For reading a portion of ROM using a0 command
 *
 *
 */
int FlashEcuSubaruMitsuM32rKline::read_mem_subaru_ecu_mitsu_kline(uint32_t start_addr, uint32_t length)
{

    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;

    uint32_t curr_addr, block_count, num_blocks;
    uint32_t tail_size, block_len, loop_ctr, block_size;

    start_addr = 0x8000;

    length = 0x78000;    // hack for testing

    block_size = 128;

    send_log_window_message("Commencing ECU ROM read", true, true);

    curr_addr = start_addr; //- 0x00100000;          // manual adjustment until fix processor details
    num_blocks = length / block_size;
    tail_size = length % block_size;
    if (tail_size != 0) num_blocks++;
    else tail_size = block_size;

    for (block_count = 0; block_count < num_blocks; block_count++)
    {
        if (block_count == (num_blocks - 1))
            block_len = tail_size;
        else
            block_len = block_size;

        msg = QString("ROM read addr: %1 length: %2").arg(curr_addr).arg(block_len).toUtf8();
        send_log_window_message(msg, true, true);

        for (loop_ctr = 0; loop_ctr < 5; loop_ctr++)
        {
            received = send_subaru_mitsu_sid_a0_block_read(curr_addr, block_len - 1);
            if (received.length() > 5) break;
        }

        if (received.length() > 5)
        {
            received.remove(0, 5);
            mapdata.append(received, (received.length() - 1));
        }

        curr_addr += block_len;
    }

    ecuCalDef->FullRomData = mapdata;

    return STATUS_SUCCESS;
}


/*
 * ECU init
 *
 * @return ECU ID and capabilities
 */
QByteArray FlashEcuSubaruMitsuM32rKline::send_subaru_mitsu_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0xBF);

    while (received == "" && loop_cnt < 1)
    {
        if (kill_process)
            break;

        send_log_window_message("SSM init", true, true);
        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
        send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);

        delay(200);
        received = serial->read_serial_data(100, receive_timeout);
        //received = QByteArray::fromHex("80f01839ffa61022ada02160000100800400000000a1462c000800000000000000dc06000829c0047e011e003e00000000000080a6e000fefe00002000da");
        send_log_window_message("Recd: " + parse_message_to_hex(received), true, true);
        loop_cnt++;
    }

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray FlashEcuSubaruMitsuM32rKline::send_subaru_mitsu_sid_81_start_communication()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    output.clear();
    output.append((uint8_t)0x81);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray FlashEcuSubaruMitsuM32rKline::send_subaru_mitsu_sid_83_request_timings()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    output.clear();
    output.append((uint8_t)0x83);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(200);

    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);

//    QMessageBox::information(this, tr("Init was OK?"), "Press OK to continue");

    return received;
}

/*
 * Request seed
 *
 * @return seed (4 bytes)
 */
QByteArray FlashEcuSubaruMitsuM32rKline::send_subaru_mitsu_sid_27_request_seed()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0x27);
    output.append((uint8_t)0x01);
    received = serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(200);
    received = serial->read_serial_data(11, receive_timeout);

    return received;
}

/*
 * Send seed key
 *
 * @return received response
 */
QByteArray FlashEcuSubaruMitsuM32rKline::send_subaru_mitsu_sid_27_send_seed_key(QByteArray seed_key)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0x27);
    output.append((uint8_t)0x02);
    output.append(seed_key);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(200);
    received = serial->read_serial_data(8, receive_timeout);

    return received;
}

/*
 * Request start diagnostic
 *
 * @return received response
 */
QByteArray FlashEcuSubaruMitsuM32rKline::send_subaru_mitsu_sid_10_start_diagnostic()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0x10);
    output.append((uint8_t)0x85);
    output.append((uint8_t)0x02);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(200);
    received = serial->read_serial_data(7, receive_timeout);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Generate denso kline seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashEcuSubaruMitsuM32rKline::subaru_ecu_mitsu_generate_kline_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex[]={
        0x8519, 0x5c53, 0xc0e9, 0x2452,
        0x1e68, 0x6feb, 0x2648, 0x81e2,
        0x8ce4, 0x953b, 0x1ca9, 0x6180,
        0xb85e, 0x5109, 0xdb3c, 0x3cf2,
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_ecu_mitsu_calculate_seed_key(requested_seed, keytogenerateindex, indextransformation);

    return key;
}

int FlashEcuSubaruMitsuM32rKline::write_mem_subaru_ecu_mitsu_kline(bool test_write)
{
    QByteArray received;
    QByteArray filedata;

    filedata = ecuCalDef->FullRomData;

    QScopedArrayPointer<uint8_t> data_array(new uint8_t[filedata.length()]);

    int block_modified[16] = {0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0};   // assume blocks after 0x8000 are modified

    unsigned bcnt;   
    unsigned blockno;

    //encrypt the data
    filedata = subaru_ecu_mitsu_encrypt_32bit_payload(filedata, filedata.length());

    for (int i = 0; i < filedata.length(); i++)
    {
        data_array[i] = filedata.at(i);
    }

    bcnt = 0;
    send_log_window_message("Blocks to flash : ", true, false);
    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
        if (block_modified[blockno]) {
            send_log_window_message(QString::number(blockno) + ", ", false, false);
            bcnt += 1;
        }
    }
    send_log_window_message(" (total: " + QString::number(bcnt) + ")", false, true);

    if (bcnt)
    {
//        send_log_window_message("--- erasing TCU flash memory ---", true, true);
//        if (erase_subaru_ecu_mitsu_can())
//        {
//            send_log_window_message("--- erasing did not complete successfully ---", true, true);
//            return STATUS_ERROR;
//        }
//        received = send_subaru_ecu_sid_83_request_timings();

        flashbytescount = 0;
        flashbytesindex = 0;

        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)
        {
            if (block_modified[blockno])
            {
                flashbytescount += flashdevices[mcu_type_index].fblocks[blockno].len;
            }
        }

        send_log_window_message("--- start writing ROM file to ECU flash memory ---", true, true);
        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)  // hack so that only 1 flash loop done for the entire ROM above 0x8000
        {
            if (block_modified[blockno])
            {
                if (reflash_block_subaru_ecu_mitsu_kline(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
                {
                    send_log_window_message("Block " + QString::number(blockno) + " reflash failed.", true, true);
                    return STATUS_ERROR;
                }
                else
                {
                    flashbytesindex += flashdevices[mcu_type_index].fblocks[blockno].len;
                    send_log_window_message("Block " + QString::number(blockno) + " reflash complete.", true, true);
                }
            }
        }

    }
    else
    {
        send_log_window_message("*** No blocks require flash! ***", true, true);
    }

    return STATUS_SUCCESS;
}

/*
 * SSM Block Write (K-Line)
 *
 * @return received response
 */

int FlashEcuSubaruMitsuM32rKline::reflash_block_subaru_ecu_mitsu_kline(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{

    int errval;

    uint32_t start_address, end_addr;
    uint32_t pl_len;
    uint16_t maxblocks;
    uint16_t blockctr;
    uint32_t blockaddr;

    QByteArray output;
    QByteArray received;
    QByteArray msg;
//    QByteArray buf;
//    uint8_t chk_sum = 0;

    set_progressbar_value(0);

    if (blockno >= fdt->numblocks) {
        send_log_window_message("block " + QString::number(blockno) + " out of range !", true, true);
        return -1;
    }

    start_address = fdt->fblocks[blockno].start;
    pl_len = fdt->fblocks[blockno].len;
    maxblocks = pl_len / 128;
    end_addr = (start_address + (maxblocks * 128)) & 0xFFFFFFFF;
    uint32_t data_len = end_addr - start_address;

    QString start_addr = QString("%1").arg((uint32_t)start_address,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)pl_len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    send_log_window_message(msg, true, true);

    send_log_window_message("Settting flash start & length...", true, true);
    qDebug() << "Settting flash start & length...";

    serial->change_port_speed("15625");
//    serial->change_port_speed("39682");

    output.clear();
    output.append((uint8_t)0x34);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0x80);
    output.append((uint8_t)0x00);

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(200);

    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);

    if (received == "" || (uint8_t)received.at(4) != 0x74)
    return STATUS_ERROR;

//    QMessageBox::information(this, tr("Init was OK?"), "Press OK to continue");

    send_log_window_message("Erasing...", true, true);

    output.clear();
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFF);

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);

    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);

    if (received == "" || (uint8_t)received.at(4) != 0x71)
    return STATUS_ERROR;

//End Of erase...
    send_log_window_message("Starting ROM Flashing...", true, true);

    int data_bytes_sent = 0;
    for (blockctr = 0; blockctr < maxblocks; blockctr++)
    {
        if (kill_process)
            return 0;

        blockaddr = start_address + blockctr * 128;
        output.clear();
        output.append((uint8_t)0x36);
        output.append((uint8_t)(blockaddr >> 16) & 0xFF);
        output.append((uint8_t)(blockaddr >> 8) & 0xFF);
        output.append((uint8_t)blockaddr & 0xFF);

        for (int i = 0; i < 128; i++)
        {
//            output[i + 8] = (uint8_t)(newdata[i + blockaddr] & 0xFF);
            output[i + 4] = (uint8_t)(newdata[i + blockaddr] & 0xFF);
            data_bytes_sent++;
        }
        data_len -= 128;

        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
//        send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
//        delay(5);
//        received = serial->read_serial_data(20, 200);
        received = serial->read_serial_data(20, receive_timeout);

//        send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);

        float pleft = (float)blockctr / (float)maxblocks * 100;
        set_progressbar_value(pleft);

    }

    set_progressbar_value(100);

//    return STATUS_SUCCESS;

    send_log_window_message("Verifying checksum...", true, true);
    qDebug() << "Verifying checksum...";

//    connected = false;
//    try_count = 0;
    delay(1000);
    output.clear();
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x02);

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);

    delay(200);
    received = serial->read_serial_data(20, 200);
        if (received != "")
        {
//            connected = true;
            send_log_window_message(": 0x31 response: " + parse_message_to_hex(received), true, true);
        }
//        try_count++;
        //delay(try_timeout);
    
    if (received == "" || (uint8_t)received.at(4) != 0x71 || (uint8_t)received.at(5) != 0x01 || (uint8_t)received.at(6) != 0x02)
        send_log_window_message("No or bad response received", true, true);
        //return STATUS_ERROR;

    send_log_window_message("Checksum verified...", true, true);
    qDebug() << "Checksum verified...";

    return STATUS_SUCCESS;

}

/*
 * SSM Byte Read (K-Line)
 *
 * @return received response
 */
QByteArray FlashEcuSubaruMitsuM32rKline::send_subaru_mitsu_sid_b8_byte_read(uint32_t dataaddr)
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0xb8);
    output.append((uint8_t)(dataaddr >> 16) & 0xFF);
    output.append((uint8_t)(dataaddr >> 8) & 0xFF);
    output.append((uint8_t)dataaddr & 0xFF);

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
   // delay(100);
    received = serial->read_serial_data(7, receive_timeout);
    //received = QByteArray::fromHex("80f018f8e0");
   // delay(100);
    send_log_window_message("Recd: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * SSM Block Read (K-Line)
 *
 * @return received response
 */
QByteArray FlashEcuSubaruMitsuM32rKline::send_subaru_mitsu_sid_a0_block_read(uint32_t dataaddr, uint32_t datalen)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    output.append((uint8_t)0xa0);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x20);
    output.append((uint8_t)(dataaddr >> 16) & 0xFF);
    output.append((uint8_t)(dataaddr >> 8) & 0xFF);
    output.append((uint8_t)dataaddr & 0xFF);
    output.append((uint8_t)datalen & 0xFF);

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);

//    received = serial->read_serial_data(datalen + 2 + 6, receive_timeout);
    received = serial->read_serial_data(7, receive_timeout);
    send_log_window_message("Recd: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Calculate subaru tcu mitsubishi seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashEcuSubaruMitsuM32rKline::subaru_ecu_mitsu_calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
{
    QByteArray key;

    uint32_t seed, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int ki, n;

    seed = (requested_seed.at(0) << 24) & 0xFF000000;
    seed += (requested_seed.at(1) << 16) & 0x00FF0000;
    seed += (requested_seed.at(2) << 8) & 0x0000FF00;
    seed += requested_seed.at(3) & 0x000000FF;
    //seed = reconst_32(seed8);

    for (ki = 15; ki >= 0; ki--) {

        wordtogenerateindex = seed;
        wordtobeencrypted = seed >> 16;
        index = wordtogenerateindex ^ keytogenerateindex[ki];
        index += index << 16;
        encryptionkey = 0;

        for (n = 0; n < 4; n++) {
            encryptionkey += indextransformation[(index >> (n * 4)) & 0x1F] << (n * 4);
        }

        encryptionkey = (encryptionkey >> 3) + (encryptionkey << 13);
        seed = (encryptionkey ^ wordtobeencrypted) + (wordtogenerateindex << 16);
    }

    seed = (seed >> 16) + (seed << 16);

    key.clear();
    key.append((uint8_t)(seed >> 24));
    key.append((uint8_t)(seed >> 16));
    key.append((uint8_t)(seed >> 8));
    key.append((uint8_t)seed);

    //write_32b(seed, key);

    return key;
}

/*
 * Encrypt upload data
 *
 * @return encrypted data
 */

QByteArray FlashEcuSubaruMitsuM32rKline::subaru_ecu_mitsu_encrypt_32bit_payload(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    const uint16_t keytogenerateindex[]={
//        0x2680, 0xca11, 0x3875, 0x25b5
        0x25b5, 0x3875, 0xca11, 0x2680
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    encrypted = subaru_ecu_mitsu_calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return encrypted;
}

QByteArray FlashEcuSubaruMitsuM32rKline::subaru_ecu_mitsu_decrypt_32bit_payload(QByteArray buf, uint32_t len)
{
    QByteArray decrypt;

    const uint16_t keytogenerateindex[]={
//	0x6587, 0x4492, 0xa8b4, 0x7bf2
      0x7c03, 0x9312, 0x2962, 0x78F1
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    decrypt = subaru_ecu_mitsu_calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return decrypt;
}

QByteArray FlashEcuSubaruMitsuM32rKline::subaru_ecu_mitsu_calculate_32bit_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
{
    QByteArray encrypted;
    uint32_t datatoencrypt32, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int ki, n;

    if (!buf.length() || !len) {
        return NULL;
    }

    encrypted.clear();

    len &= ~3;
    for (uint32_t i = 0; i < len; i += 4) {
        datatoencrypt32 = ((buf.at(i) << 24) & 0xFF000000) | ((buf.at(i + 1) << 16) & 0xFF0000) | ((buf.at(i + 2) << 8) & 0xFF00) | (buf.at(i + 3) & 0xFF);

        for (ki = 0; ki < 4; ki++) {

            wordtogenerateindex = datatoencrypt32;
            wordtobeencrypted = datatoencrypt32 >> 16;
            index = wordtogenerateindex ^ keytogenerateindex[ki];
            index += index << 16;
            encryptionkey = 0;

            for (n = 0; n < 4; n++) {
                encryptionkey += indextransformation[(index >> (n * 4)) & 0x1F] << (n * 4);
            }

            encryptionkey = (encryptionkey >> 3) + (encryptionkey << 13);
            datatoencrypt32 = (encryptionkey ^ wordtobeencrypted) + (wordtogenerateindex << 16);
        }

        datatoencrypt32 = (datatoencrypt32 >> 16) + (datatoencrypt32 << 16);

        encrypted.append((datatoencrypt32 >> 24) & 0xFF);
        encrypted.append((datatoencrypt32 >> 16) & 0xFF);
        encrypted.append((datatoencrypt32 >> 8) & 0xFF);
        encrypted.append(datatoencrypt32 & 0xFF);
        //encrypted.append(sub_encrypt(tempbuf));
    }
    return encrypted;
}

/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashEcuSubaruMitsuM32rKline::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
{
    QByteArray received;
    QByteArray msg;

    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, target_id & 0xFF);
    output.insert(2, tester_id & 0xFF);
    output.insert(3, length);

    output.append(calculate_checksum(output, dec_0x100));

    //send_log_window_message("Send: " + parse_message_to_hex(output), true, true);
    //qDebug () << "Send:" << parse_message_to_hex(output);
    return output;
}

/*
 * Calculate SSM checksum to message
 *
 * @return 8-bit checksum
 */
uint8_t FlashEcuSubaruMitsuM32rKline::calculate_checksum(QByteArray output, bool dec_0x100)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
        checksum += (uint8_t)output.at(i);

    if (dec_0x100)
        checksum = (uint8_t) (0x100 - checksum);

    return checksum;
}


/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashEcuSubaruMitsuM32rKline::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}


/*
 * Output text to log window
 *
 * @return
 */
int FlashEcuSubaruMitsuM32rKline::send_log_window_message(QString message, bool timestamp, bool linefeed)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']  ");

    if (timestamp)
        message = dateTimeString + message;
    if (linefeed)
        message = message + "\n";

    QTextEdit* textedit = this->findChild<QTextEdit*>("text_edit");
    if (textedit)
    {
        ui->text_edit->insertPlainText(message);
        ui->text_edit->ensureCursorVisible();

        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

void FlashEcuSubaruMitsuM32rKline::set_progressbar_value(int value)
{
    if (ui->progressbar)
        ui->progressbar->setValue(value);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void FlashEcuSubaruMitsuM32rKline::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

