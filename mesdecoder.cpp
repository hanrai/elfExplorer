#include "mesdecoder.h"
#include <QStack>
#include <QtDebug>

MesDecoder::MesDecoder(QObject *parent) : QObject(parent)
{

}

QString MesDecoder::Decode(QByteArray src)
{
    buffer.setData(src);
    buffer.open(QBuffer::ReadOnly);

    result = "{\n";
    while(!buffer.atEnd())
    {
        QString pos = QString::number(buffer.pos());
        result += pos+":\t";

        char command;
        buffer.getChar(&command);

        switch(command)
        {
        case 0x0:
            result+="return;\n";
            break;
        case 0x1:
        {
            QString txt;
            while(1)
            {
                quint16 l = GetMes();
                if(!l)
                    break;
                quint16 r = GetMes();
                quint16 res=0;
                if((l<0x81 || l>0xFE) && (l<0xE0 || l>0xEF))
                    res = r<<8;
                else
                    res = l | r<<8;

                txt += QString::fromLocal8Bit((char*)&res,2);
            }
            result+="Text1(\""+txt+"\");\n";
        }
            break;
        case 0x2:
        {
            QString txt;
            while(1)
            {
                quint8 l = GetMes();
                if(!l)
                    break;
                txt+=QString::fromLocal8Bit((char*)&l,1);
            }
            result+="Text2(\""+txt+"\");\n";
        }
            break;
        case 0x3:
            {
                quint16 index = 0;
                index = GetMes();
                index |= GetMes()<<8;
                do
                {
                    Node n = Calculate();
                    result += GetFlags(index)+"=";
                    if(n.isNumber)
                        result += QString::number(qMin(0xF, n.number))+";\n";
                    else
                        result += "qmin(0xF, "+n.str+");\n";
                    index++;
                }while(GetMes());
            }
            break;
        case 0x4:
            {
                quint8 index = GetMes();
                do
                {
                    result+=GetReg16(index)+"="+Calculate().str+";\n";
                    index++;
                }while(GetMes());
            }
            break;
        case 0x5:
        {
            Node idx = Calculate();
            for(int i = 0;;i++)
            {
                if(i>0)
                    result += "\t";
                result += GetFlags(idx, i)+"="+Calculate().str+";\n";
                if(!GetMes())
                    break;
            }
        }
            break;
        case 0x7:
            {
                Node idx = Calculate();
                quint8 selector = GetMes();
                quint32 index = 0;
                if(selector)
                {
                    do{
                        QString i = idx.str;
                        if(index>0)
                        {
                            result += "\t";
                            if(idx.isNumber)
                                i = QString::number(idx.number+index);
                            else
                                i = idx.str+"+"+QString::number(index);
                        }
                        result += "(quint16*)((char*)&mainStruct+"+GetReg16(selector-1)+")["+i+"]="+Calculate().str+";\n";
                        index++;
                    }while(GetMes());
                }
                else
                {
                    do{
                        if(index>0)
                            result += "\t";
                        result += GetDim16(idx, index++)+"=";
                        Node n = Calculate();

                        if(!idx.isNumber || !n.isNumber)
                        {
                            result += n.str+";\n";
                            continue;
                        }

                        if(idx.number+index<100 || idx.number+index>360)
                        {
                            result += n.str+";\n";
                            continue;
                        }

                        quint16 c = n.number;
                        QString str = QString::fromLocal8Bit((char*)&c,2);
                        result += n.str+":"+str+";\n";

                    }while(GetMes());
                }
            }
            break;
        case 0x8:
            {
                Node idx = Calculate();
                quint8 selector = GetMes();
                quint32 index = 0;
                if(selector)
                {
                    do{
                        QString i = idx.str;
                        if(index>0)
                        {
                            result += "\t";
                            if(idx.isNumber)
                                i = QString::number(idx.number+index);
                            else
                                i = idx.str+"+"+QString::number(index);
                        }
                        result += "(quint32*)((char*)&mainStruct+"+GetReg32(selector-1)+")["+i+"]="+Calculate().str+";\n";
                        index++;
                    }while(GetMes());
                }
                else
                {
                    do{
                        if(index>0)
                            result += "\t";
                        result += GetDim32(idx, index++)+"="+Calculate().str+";\n";
                    }while(GetMes());
                }
            }
            break;
        case 0x9:
            {
            result += "if("+Calculate().str+"!=1)\n";
                ByteWorker offset;
                offset.byte[0]=GetMes();
                offset.byte[1]=GetMes();
                offset.byte[2]=GetMes();
                offset.byte[3]=GetMes();
                result+="\t goto "+QString::number(offset.dword)+";\n";
            }
            break;
        case 0xA:
            {
                ByteWorker offset;
                offset.byte[0]=GetMes();
                offset.byte[1]=GetMes();
                offset.byte[2]=GetMes();
                offset.byte[3]=GetMes();
                result+="goto "+QString::number(offset.dword)+";\n";
            }
            break;
        case 0xB:
            GameCommand();
            break;
        case 0xC:
            PrepareSubCommands();
            result += "ComChange(filename=\""+QString((char*)subCommands[0].buffer)+"\");\n";
            break;
        case 0xD:
            PrepareSubCommands();
            result += "CallMes(filename=\""+QString((char*)subCommands[0].buffer)+"\");\n";
            break;
        case 0xE:
        {
            PrepareSubCommands();
            ByteWorker bw;
            bw.byte[0]=GetMes();
            bw.byte[1]=GetMes();
            bw.byte[2]=GetMes();
            bw.byte[3]=GetMes();
            result += "SetBookmark(index="+subCommands[0].command.str+", offset="+GetNumber((quint32)bw.dword)+");\n";
        }
            break;
        case 0xF:
            PrepareSubCommands();
            result += "RunFromSavedAnchorPoint(id="+subCommands[0].command.str+");\n";
            break;
        case 0x10:
            PaletteMisc();
            break;
        case 0x11:
            result += "NewLine();\n";
            break;
        case 0x12:
        {
            auto index = Calculate().str;
            ByteWorker offset;
            offset.byte[0]=GetMes();
            offset.byte[1]=GetMes();
            offset.byte[2]=GetMes();
            offset.byte[3]=GetMes();
            result +="AnchorIndexs["+index+"].cursor = "+QString::number(buffer.pos())+";\n";
            result +="\toffset = "+QString::number(offset.dword)+";\n";
        }
            break;
        case 0x13:
            result += "UserInput();\n";
            //StartCommand("UserInput", pos);
            break;
        case 0x14:
            {
            for(auto index = GetMes();;index++)
                {
                    result += GetReg32(index)+"="+Calculate().str+";\n";
                    if(!GetMes())
                        break;
                }
            }
            break;
        default:
            if ((command >= 0x81 && command <= 0xFE)
              ||(command >= 0xE0 && command <= 0xEF))
            {
                QString txt;
                while(1)
                {
                    quint16 l = GetMes();
                    if(!l)
                        break;
                    quint16 r = GetMes();
                    quint16 res=0;
                    if((l<0x81 || l>0xFE) && (l<0xE0 || l>0xEF))
                        res = r<<8;
                    else
                        res = l | r<<8;

                    txt += QString::fromLocal8Bit((char*)&res,2);
                }
                result+="DefaultTxt(\""+txt+"\");\n";
            }
            else
                result+=QString("UnknownCommand_%0\n").arg(GetNumber(quint8(command)));
            break;
        }
    }
    result += "}\n";

    buffer.close();
    return result;
}

quint8 MesDecoder::GetMes()
{
    quint8 result;
    buffer.getChar((char*)&result);
    return result;
}

Node MesDecoder::Calculate()
{
    bool isNotFinished = true;

    QStack<Node> stack;
    do
    {
        quint8 command = GetMes();
        switch(command)
        {
        case 0x80:
        {
            quint8 i = GetMes();
            Node n;
            n.isNumber = false;
            n.number = i;
            n.str = GetReg16(i);
            stack.push(n);
        }
            break;
        case 0xA0:
        {
            Q_ASSERT(stack.size()>0);
            Node n;
            n.isNumber = false;

            quint8 selector = GetMes();
            Node index = stack.pop();

            if(selector)
                n.str = QString("(quint16*)((char*)Mainstruct+")+GetReg16(selector-1)+")["+index.str+"]";
            else
                n.str = GetDim16(index);
            stack.push(n);
        }
            break;
        case 0xC0:
        {
            Q_ASSERT(stack.size()>0);
            Node n;
            n.isNumber = false;
            Node index = stack.pop();
            n.str = "*((char*)MainStruct+"+index.str+"+"+GetReg16(GetMes())+")";
            stack.push(n);
        }
            break;
        case 0xE0:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node l = stack.pop();
            Node r = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number+r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+r.str+"+"+l.str+")";
            stack.push(n);
        }
            break;
        case 0xE1:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node r = stack.pop();
            Node l = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number-r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+l.str+"-"+r.str+")";
            stack.push(n);
        }
            break;
        case 0xE2:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node l = stack.pop();
            Node r = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number*r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+l.str+"*"+r.str+")";
            stack.push(n);
        }
            break;
        case 0xE3:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node r=stack.pop();
            Node l=stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number/r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+l.str+"/"+r.str+")";
            stack.push(n);
        }
            break;
        case 0xE4:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node r=stack.pop();
            Node l=stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number%r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+l.str+"%"+r.str+")";
            stack.push(n);
        }
            break;
        case 0xE5:
        {
            quint32 res = GetMes() | GetMes()<<8;
            Node n;
            n.isNumber = false;
            n.str = "(random()%"+QString::number(res)+")";
            stack.push(n);
        }
            break;
        case 0xE6:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node l = stack.pop();
            Node r = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = (l.number==1) && (r.number==1);
            }
            else
                n.isNumber = false;
            n.str = "("+l.str+"==1 && "+r.str+"==1)";
            stack.push(n);
        }
            break;
        case 0xE7:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node l = stack.pop();
            Node r = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = (l.number==1) || (r.number==1);
            }
            else
                n.isNumber = false;
            n.str = "("+l.str+"==1 || "+r.str+"==1)";
            stack.push(n);
        }
            break;
        case 0xE8:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node l = stack.pop();
            Node r = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number&r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+l.str+"&"+r.str+")";
            stack.push(n);
        }
            break;
        case 0xE9:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node r = stack.pop();
            Node l = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number|r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+l.str+"|"+r.str+")";
            stack.push(n);
        }
            break;
        case 0xEA:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node l = stack.pop();
            Node r = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number^r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+l.str+"^"+r.str+")";
            stack.push(n);
        }
            break;
        case 0xEB:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node l = stack.pop();
            Node r = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number>r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+r.str+"<"+l.str+")";
            stack.push(n);
        }
            break;
        case 0xEC:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node l = stack.pop();
            Node r = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number<r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+r.str+">"+l.str+")";
            stack.push(n);
        }
            break;
        case 0xED:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node l = stack.pop();
            Node r = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number>=r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+r.str+"<="+l.str+")";
            stack.push(n);
        }
            break;
        case 0xEE:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node l = stack.pop();
            Node r = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number<=r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+r.str+">="+l.str+")";
            stack.push(n);
        }
            break;
        case 0xEF:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node r = stack.pop();
            Node l = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number==r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+l.str+"=="+r.str+")";
            stack.push(n);
        }
            break;
        case 0xF0:
        {
            Q_ASSERT(stack.size()>1);
            Node n;
            Node r = stack.pop();
            Node l = stack.pop();
            if(l.isNumber && r.isNumber)
            {
                n.isNumber = true;
                n.number = l.number!=r.number;
            }
            else
                n.isNumber = false;
            n.str = "("+l.str+"!="+r.str+")";
            stack.push(n);
        }
            break;
        case 0xF1:
        {
            Node n;
            n.isNumber = true;
            quint32 l=GetMes();
            quint32 r=GetMes();
            n.number = l|r<<8;
            n.str = GetNumber(n.number);
            stack.push(n);
        }
            break;
        case 0xF2:
        {
            Node n;
            n.isNumber = true;
            ByteWorker bw;
            bw.byte[0]=GetMes();
            bw.byte[1]=GetMes();
            bw.byte[2]=GetMes();
            bw.byte[3]=GetMes();
            n.number = bw.dword;
            n.str = GetNumber(bw.dword);
            stack.push(n);
        }
            break;
        case 0xF3:
        {
            Node n;
            n.isNumber = false;
            ByteWorker bw;
            bw.dword=0;
            bw.byte[0]=GetMes();
            bw.byte[1]=GetMes();
            n.number = bw.dword;
            n.str = GetFlags(bw.dword);
            stack.push(n);
        }
            break;
        case 0xF4:
        {
            Node n;
            Node index = stack.pop();
            n.isNumber = false;
            n.str = GetFlags(index);
            stack.push(n);
        }
            break;
        case 0xF5:
        {
            Q_ASSERT(stack.size()>0);
            Node n;
            n.isNumber = false;

            quint8 selector = GetMes();
            Node index = stack.pop();

            if(selector)
                n.str = "((quint32*)"+GetReg32(selector-1)+")["+index.str+"]";
            else
                n.str = GetDim32(index);
            stack.push(n);
        }
            break;
        case 0xF6:
        {
            Node n;
            n.isNumber = false;
            n.str = GetDim32(GetMes());
            stack.push(n);
        }
            break;
        case 0xFF:
        {
            isNotFinished = false;
        }
            break;
        default:
        {
            Node n;
            n.isNumber = true;
            n.number = command;
            n.str = QString::number(command);
            stack.push(n);
        }
            break;
        }
    }while(isNotFinished);
    Q_ASSERT(stack.size()>0);
    return stack.pop();
}

void MesDecoder::PrepareSubCommands()
{
    subCommandIndex = 0;
    for(auto i=0; i<30; i++)
    {
        memset(subCommands[i].buffer, 0, 23);
        subCommands[i].type = 0;
        subCommands[i].command.isNumber = false;
        subCommands[i].command.number = 0;
        subCommands[i].command.str.clear();
    }

    forever
    {
        char command = GetMes();
        if(!command)
            break;
        if(command==1)
        {
            int i=0;
            subCommands[subCommandIndex].type=1;
            forever
            {
                char c=GetMes();
                if(!c)
                    break;
                subCommands[subCommandIndex].buffer[i++]=c;
            }
        }
        else
        {
            if(command==2)
            {
                subCommands[subCommandIndex].type=2;
                subCommands[subCommandIndex].command=Calculate();
            }
        }
        subCommandIndex++;
    }
}

void MesDecoder::GameCommand()
{
    Node command = Calculate();
    PrepareSubCommands();

    if(!command.isNumber)
    {
        result += "Unknown GameCommand:"+command.str+";\n";
        return;
    }

    switch(command.number)
    {
    case 0:
        result += "CreateNewFont();\n";
        break;
    case 1:
        result += "DrawNumber("+subCommands[0].command.str+");\n";
        break;
    case 2:
        Cursor();
        break;
    case 3:
        Animation();
        break;
    case 4:
        Saved();
        break;
    case 5:
        Sound();
        break;
    case 6:
        result += QString("Voice.play(filename=\"%0\");\n").arg((char*)subCommands[1].buffer);;
        break;
    case 7:
        result += QString("LoadDataMes(filename=\"%0\", offset=%1);\n")
                .arg((char*)subCommands[0].buffer)
                .arg(subCommands[1].command.str);
        break;
    case 8:
        result += QString("DrawImage(filename=\"%0\");\t//Draw on Flag.vRamIndex\n").arg((char*)subCommands[0].buffer);
        break;
    case 9:
        Palette();
        break;
    case 10:
        Painting();
        break;
    case 11:
        if(subCommandIndex)
        {
            if(subCommands[0].command.isNumber)
                result += "SkipableWait(timeout="+QString::number(subCommands[0].command.number>>2)+");\n";
            else
                result += "SkipableWait(timeout="+subCommands[0].command.str+">>2);\n";
        }
        else
            result += "SkipableWait();\n";
        break;
    case 12:
        if(subCommands[0].command.isNumber)
            result += "SetColor(bgColor="+QString::number((subCommands[0].command.number&0xF0)>>4)
                    +", txtColor="+QString::number(subCommands[0].command.number&0xF)+");\n";
        else
            result += "SetColor(bgColor=("+subCommands[0].command.str+"&0xF0)>>4, txtColor="
                    +subCommands[0].command.str+"&0xF);\n";
        break;
    case 13:
        result += "RunMes(offset="+subCommands[0].command.str+");\n";
        break;
    case 14:
        result += "SetMoveResult(begin="+subCommands[0].command.str+", end="+subCommands[1].command.str+");\n";
        break;
    case 15:
        if(subCommands[0].command.isNumber)
            result += "SetBookmarkIndex("+QString::number(subCommands[0].command.number&0xF)+");\n";
        else
            result += "SetBookmarkIndex("+subCommands[0].command.str+"&0xF);\n";
        break;
    case 16:
        result += "RandomReg16bitTopArea();\n";
        break;
    case 17:
        Move();
        break;
    case 18:
        Cache();
        break;
    case 19:
        result += "ReloadFile("+subCommands[0].command.str+","+subCommands[1].command.str+");\n";
        break;
    case 20:
        result += "WatiInput();\n";
        break;
    default:
        result += "Unknown GameCommand:"+command.str+";\n";
    }
}

QString MesDecoder::GetNumber(int number)
{
    return QString::number(number);
    //屏蔽
    QString n = QString().setNum(number);
    n+= QString("(0x%0)").arg(QString::number(number, 16).toUpper());
    return n;
}

QString MesDecoder::GetDim16(quint32 index)
{
    QStringList dim16;
    dim16 << "B10offset"
        << "vRamIndex"
        << "field_4"
        << "mainFlag"
        << "cursorLeft"
        << "cursorTop"
        << "winLeft"
        << "winTop"
        << "winWidth"
        << "winHeight"
        << "txtLeft"
        << "txtTop"
        << "colorIndex"
        << "flagDigPosComb"
        << "font_double_width"
        << "font_height"
        << "drawTextStyle"
        << "txtWidth"
        << "txtHeight"
        << "posTop"
        << "posLeft"
        << "left"
        << "top"
        << "width"
        << "height"
        << "colorMaskIndex"
        << "BookmarkIndex"
        << "field_8EE";

    return Readable("dim16bit", dim16, index);
}

QString MesDecoder::GetDim16(Node &node)
{
    if(node.isNumber)
    {
        if(node.number>=100 && node.number<=360)
            return "charMap["+QString::number(node.number-100)+"]";
        return GetDim16(node.number);
    }
    else
        return "dim16Bit["+node.str+"]";
}

QString MesDecoder::GetDim16(Node node, qint32 i)
{
    if(node.isNumber)
        node.number += i;
    else
        if(i!=0)
            node.str += "+"+QString::number(i);
    return GetDim16(node);
}

QString MesDecoder::GetReg16(quint32 index)
{
    QStringList reg16;
    reg16<<"tempReg"
         <<"system_Month"
         <<"system_DayOfWeek"
         <<"system_Day"
         <<"dayOfWeek"
         <<"system_Minute"
         <<"system_Second"
         <<"date"
         <<"index"
         <<"EF8offset"
         <<"field_14"
         <<"field_16"
         <<"money"
         <<"select"
         <<"field_1C"
         <<"field_1E"
         <<"field_20"
         <<"field_22"
         <<"selectedItem"
         <<"time"
         <<"field_28"
         <<"field_2A"
         <<"field_2C"
         <<"destLeft"
         <<"destTop"
         <<"field_32";
    return Readable("reg16bit", reg16, index);
}

QString MesDecoder::GetReg16(Node &node)
{
    if(node.isNumber)
        return GetReg16(node.number);
    else
        return "reg16bit["+node.str+"]";
}

QString MesDecoder::GetReg16(Node node, qint32 i)
{
    if(node.isNumber)
    {
        node.number += i;
        node.str = QString::number(node.number);
    }
    else
        if(i!=0)
            node.str += "+"+QString::number(i);
    return GetReg16(node);
}

QString MesDecoder::GetFlags(quint32 index)
{
    switch(index)
    {
    case 577:
        return QString("Flag.577random");
    case 2047:
        return QString("Flag.2047mapArea");
    case 406:
        return QString("Flag.406knownAzumi");
    default:
        return "Flag["+QString::number(index)+"]";
    }
}

QString MesDecoder::GetFlags(Node &node)
{
    if(node.isNumber)
        return GetFlags(node.number);
    else
        return "Flags["+node.str+"]";
}

QString MesDecoder::GetFlags(Node node, qint32 i)
{
    if(node.isNumber)
        node.number += i;
    else
        if(i!=0)
            node.str += "+"+QString::number(i);
    return GetFlags(node);
}

QString MesDecoder::GetReg32(quint32 index)
{
    QStringList list;
    list<<"field_0"
        <<"field_4"
        <<"field_8"
        <<"field_C"
        <<"field_10"
        <<"field_14"
        <<"field_18"
        <<"field_1C"
        <<"field_20"
        <<"field_24"
        <<"field_28"
        <<"field_2C"
        <<"field_30"
        <<"field_34"
        <<"field_38"
        <<"field_3C"
        <<"field_40"
        <<"musicID"
        <<"field_48"
        <<"field_4C"
        <<"field_50"
        <<"field_54"
        <<"field_58"
        <<"field_5C"
        <<"field_60"
        <<"field_64";
    return Readable("dim32bit", list, index);
}

QString MesDecoder::GetReg32(Node &node)
{
    if(node.isNumber)
        return GetReg32(node.number);
    else
        return QString("reg32bit["+node.str+"]");
}

QString MesDecoder::GetReg32(Node node, qint32 i)
{
    if(node.isNumber)
        node.number += i;
    else
        if(i!=0)
            node.str += "+"+QString::number(i);
    return GetReg32(node);
}

QString MesDecoder::GetDim32(Node &node)
{
    if(node.isNumber)
        return GetDim32(node.number);
    else
        return QString("dim32bit["+node.str+"]");
}

QString MesDecoder::GetDim32(Node node, qint32 i)
{
    if(node.isNumber)
        node.number += i;
    else
        if(i!=0)
            node.str += "+"+QString::number(i);
    return GetDim32(node);
}

QString MesDecoder::GetDim32(quint32 index)
{
    QStringList list;
    list<<"defmainOffset"
        <<"animationOffset"
        <<"mapDataOffset"
        <<"moveInfoBase"
        <<"field_10"
        <<"field_14"
        <<"scriptBase"
        <<"pMesCursorBookmarks"
        <<"field_20";
    return Readable("reg32bit", list, index);
}

QString MesDecoder::Readable(QString name, QStringList list, quint32 index)
{
    if(index>=list.size())
        return name+"["+QString::number(index)+"]";
    return name+"."+list[index];
}

void MesDecoder::Cursor()
{
    if(!subCommands[0].command.isNumber)
    {
        result += "Unknown Cursor Command:"+subCommands[0].command.str+";\n";
        return;
    }
    switch(subCommands[0].command.number)
    {
    case 0:
        result += "Cursor.show();\n";
        break;
    case 1:
        result += "Cursor.hide();\n";
        break;
    case 2:
        result += "Cursor.clip(left="+subCommands[1].command.str+", " +
                "top="+subCommands[2].command.str+", "+
                "right="+subCommands[3].command.str+", "+
                "bottom="+subCommands[4].command.str+");\n";
        break;
    case 3:
        result += "Cursor.position();//set dim16bit.cursorLeft and dim16bit.cursorTop\n";
        break;
    case 4:
        result += "Cursor.setPosition(left="+subCommands[1].command.str+", "+
                "top="+subCommands[2].command.str+");\n";
        break;
    case 5:
        result += "Cursor.setIcon(id="+subCommands[1].command.str+");\n";
        break;
    default:
        result += "Unknown Cursor command:"+subCommands[0].command.str+"\n";
        break;
    }
}

void MesDecoder::Animation()
{
    if(!subCommands[0].command.isNumber)
    {
        result += "Unknown Animation command:"+subCommands[0].command.str+"\n";
        return;
    }
    switch(subCommands[0].command.number)
    {
    case 0:
        result += "Animation.prepare(id="+subCommands[1].command.str+");\n";
        break;
    case 1:
        result += "Animation.playLoop(id="+subCommands[1].command.str+");\n";
        break;
    case 2:
        result += "Animation.play(id="+subCommands[1].command.str+");\n";
        break;
    case 3:
        result += "Animation.stop(id="+subCommands[1].command.str+");\n";
        break;
    case 4:
        result += "Animation.playWait(id="+subCommands[1].command.str+");\n";
        break;
    case 5:
        result += "Animation.resetAllCount();\n";
        break;
    case 6:
        result += "Animation.stopAll();\n";
        break;
    case 7:
        result += "Animation.playAll();\n";
        break;
    default:
        result += "Unknown Animation command:"+subCommands[0].command.str+"\n";
        break;
    }
}

void MesDecoder::Saved()
{
    if(!subCommands[0].command.isNumber)
    {
        result += "Unknown Saved game command:"+subCommands[0].command.str+"\n";
        return;
    }
    switch(subCommands[0].command.number)
    {
    case 0:
        result += "Saved.load(filename=\"SAVE0"+subCommands[1].command.str+"\");\n";
        break;
    case 1:
        result += "Saved.save(filename=\"SAVE0"+subCommands[1].command.str+"\");\n";
        break;
    case 2:
        result += "Saved.loadHalf(filename=\"SAVE0"+subCommands[1].command.str+"\");//Keep Scene\n";
        break;
    case 3:
        result += "Saved.saveHalf(filename=\"SAVE0"+subCommands[1].command.str+"\");\n";
        break;
    default:
        result += "Unknown Saved game command:"+subCommands[0].command.str+"\n";
        break;
    }
}

void MesDecoder::Sound()
{
    if(!subCommands[0].command.isNumber)
    {
        result += "Unknown Sound command:"+subCommands[0].command.str+"\n";
        return;
    }
    switch(subCommands[0].command.number)
    {
    case 0:
        result += QString("Sound.playBGM(filename=\"%0\");\n").arg((char*)subCommands[1].buffer);
        break;
    case 1:
        result += "Sound.stopBGM();\n";
        break;
    case 2:
        result += "Sound.fadeOutBGM();\n";
        break;
    case 3:
        result += QString("Sound.play(filename=\"%0\");\n").arg((char*)subCommands[1].buffer);
        break;
    case 4:
        result += "Sound.stop();\n";
        break;
    default:
        result += "Unknown Sound command:"+subCommands[0].command.str+"\n";
        break;
    }
}

void MesDecoder::Palette()
{
    if(!subCommands[0].command.isNumber)
    {
        result += "Unknown Palette command:"+subCommands[0].command.str+"\n";
        return;
    }
    switch(subCommands[0].command.number)
    {
    case 0:
        if(subCommandIndex>1)
            result += "Palette.initLogPalette(color="+subCommands[1].command.str+");\n";
        else
            result += "Palette.initLogPalette(paletteBuffer);\n";
        break;
    case 1:
        if(subCommandIndex>1)
            result += "Palette.initTargetPalette(color="+subCommands[1].command.str+");\n";
        else
            result += "Palette.initTargetPalette(paletteBuffer);\n";
        break;
    case 2:
        result += "Palette.playWait(start="+subCommands[1].command.str+", steps="
                +subCommands[2].command.str+");\n";
        break;
    case 3:
        result += "Palette.fadeIn(initColor="+subCommands[1].command.str+", start="
                +subCommands[2].command.str+", count="
                +subCommands[3].command.str+");\n";
        break;
    case 4:
        if(subCommandIndex>1)
            result += "Palette.fadeOut(targetColor="+subCommands[1].command.str+");\n";
        else
            result += "Plaette.fadeOut(paletteBuffer);\n";
        break;
    case 5:
        result += "Palette.setLogFromTarget(start="+subCommands[1].command.str
                +", count="+subCommands[2].command.str+");\n";
        break;
    case 6:
        result += "Palette.play(start="+subCommands[1].command.str+", count="
                +subCommands[2].command.str+");\n";
        break;
    case 7:
        if(subCommandIndex>2)
            result += "Palette.fadeOutNStep(targetColor="+subCommands[2].command.str
                    +", steps="+subCommands[1].command.str+");\n";
        else
            result += "Palette.fadeOutNStep(steps="+subCommands[1].command.str+");\n";
        break;
    default:
        result += "Unknown Palette command:"+subCommands[0].command.str+"\n";
        break;
    }
}

void MesDecoder::Painting()
{
    if(!subCommands[0].command.isNumber)
    {
        result += "Unknown Palette command:"+subCommands[0].command.str+"\n";
        return;
    }
    switch(subCommands[0].command.number)
    {
    case 0:
        result += "Painting.srcToTarget(\n\tsrcVram="+subCommands[5].command.str
                +", \n\tsrcLeft="+subCommands[1].command.str
                +", \n\tsrcTop="+subCommands[2].command.str
                +", \n\tsrcWidth="+(subCommands[3].command.isNumber&&subCommands[1].command.isNumber?QString::number(subCommands[3].command.number-subCommands[1].command.number+1):subCommands[3].command.str+"-"+subCommands[1].command.str+"+1")
                +", \n\tsrcHeight="+(subCommands[4].command.isNumber&&subCommands[2].command.isNumber?QString::number(subCommands[4].command.number-subCommands[2].command.number+1):subCommands[4].command.str+"-"+subCommands[2].command.str+"+1")
                +", \n\tdestVram="+subCommands[8].command.str
                +", \n\tdestTop="+subCommands[7].command.str
                +", \n\tdestLeft="+subCommands[6].command.str
                +");\n";
        break;
    case 1:
        result += "Painting.srcToTargetWithMask(\n\tsrcVram="+subCommands[5].command.str
                +", \n\tsrcLeft="+subCommands[1].command.str
                +", \n\tsrcTop="+subCommands[2].command.str
                +", \n\tsrcWidth="+(subCommands[3].command.isNumber&&subCommands[1].command.isNumber?QString::number(subCommands[3].command.number-subCommands[1].command.number+1):subCommands[3].command.str+"-"+subCommands[1].command.str+"+1")
                +", \n\tsrcHeight="+(subCommands[4].command.isNumber&&subCommands[2].command.isNumber?QString::number(subCommands[4].command.number-subCommands[2].command.number+1):subCommands[4].command.str+"-"+subCommands[2].command.str+"+1")
                +", \n\tdestVram="+subCommands[8].command.str
                +", \n\tdestTop="+subCommands[7].command.str
                +", \n\tdestLeft="+subCommands[6].command.str
                +", \n\tmask=dim16Bit.colorMaskIndex);\n";
        break;
    case 2:
        result += "Painting.fillBackground(\n\tvram=dim16bit.vRamIndex, \n\tleft="
                +subCommands[1].command.str
                +", \n\ttop="+subCommands[2].command.str
                +", \n\twidth="+(subCommands[3].command.isNumber&&subCommands[1].command.isNumber?QString::number(subCommands[3].command.number-subCommands[1].command.number+1):subCommands[3].command.str+"-"+subCommands[1].command.str+"+1")
                +", \n\theight="+(subCommands[4].command.isNumber&&subCommands[2].command.isNumber?QString::number(subCommands[4].command.number-subCommands[2].command.number+1):subCommands[4].command.str+"-"+subCommands[2].command.str+"+1")
                +", \n\tcolor=dim16Bit.colorIndex);\n";
        break;
    case 3:
        result += "Painting.switchImgs(\n\tsrcVram="+subCommands[5].command.str
                +", \n\tsrcLeft="+subCommands[1].command.str
                +", \n\tsrcTop="+subCommands[2].command.str
                +", \n\tsrcWidth="+(subCommands[3].command.isNumber&&subCommands[1].command.isNumber?QString::number(subCommands[3].command.number-subCommands[1].command.number+1):subCommands[3].command.str+"-"+subCommands[1].command.str+"+1")
                +", \n\tsrcHeight="+(subCommands[4].command.isNumber&&subCommands[2].command.isNumber?QString::number(subCommands[4].command.number-subCommands[2].command.number+1):subCommands[4].command.str+"-"+subCommands[2].command.str+"+1")
                +", \n\tdestVram="+subCommands[8].command.str
                +", \n\tdestTop="+subCommands[7].command.str
                +", \n\tdestLeft="+subCommands[6].command.str
                +");\n";
        break;
    case 4:
        result += "Painting.drawHighlight(\n\tvram=dim16bit.vRamIndex, \n\tleft="
                +subCommands[1].command.str
                +", \n\ttop="+subCommands[2].command.str
                +", \n\twidth="+(subCommands[3].command.isNumber&&subCommands[1].command.isNumber?QString::number(subCommands[3].command.number-subCommands[1].command.number+1):subCommands[3].command.str+"-"+subCommands[1].command.str+"+1")
                +", \n\theight="+(subCommands[4].command.isNumber&&subCommands[2].command.isNumber?QString::number(subCommands[4].command.number-subCommands[2].command.number+1):subCommands[4].command.str+"-"+subCommands[2].command.str+"+1")
                +", \n\tcolor=dim16Bit.colorIndex);\n";
        break;
    case 5:
        result += "Painting.slideEffect(\n\tsrcVram="+subCommands[5].command.str
                +", \n\tsrcLeft="+subCommands[1].command.str
                +", \n\tsrcTop="+subCommands[2].command.str
                +", \n\tsrcWidth="+(subCommands[3].command.isNumber&&subCommands[1].command.isNumber?QString::number(subCommands[3].command.number-subCommands[1].command.number+1):subCommands[3].command.str+"-"+subCommands[1].command.str+"+1")
                +", \n\tsrcHeight="+(subCommands[4].command.isNumber&&subCommands[2].command.isNumber?QString::number(subCommands[4].command.number-subCommands[2].command.number+1):subCommands[4].command.str+"-"+subCommands[2].command.str+"+1")
                +", \n\tdestVram="+subCommands[8].command.str
                +", \n\tdestTop="+subCommands[7].command.str
                +", \n\tdestLeft="+subCommands[6].command.str
                +");\n";
        break;
    case 6:
        result += "Painting.mixWithMask(\n\tsrcVram="+subCommands[5].command.str
                +", \n\tsrcLeft="+subCommands[1].command.str
                +", \n\tsrcTop="+subCommands[2].command.str
                +", \n\tsrcWidth="+(subCommands[3].command.isNumber&&subCommands[1].command.isNumber?QString::number(subCommands[3].command.number-subCommands[1].command.number+1):subCommands[3].command.str+"-"+subCommands[1].command.str+"+1")
                +", \n\tsrcHeight="+(subCommands[4].command.isNumber&&subCommands[2].command.isNumber?QString::number(subCommands[4].command.number-subCommands[2].command.number+1):subCommands[4].command.str+"-"+subCommands[2].command.str+"+1")
                +", \n\tdestVram="+subCommands[8].command.str
                +", \n\tdestTop="+subCommands[7].command.str
                +", \n\tdestLeft="+subCommands[6].command.str
                +", \n\tfinalVram="+subCommands[11].command.str
                +", \n\tfinalTop="+subCommands[9].command.str
                +", \n\tfinalLeft="+subCommands[10].command.str
                +", \n\tmask=dim16Bit.colorMaskIndex);\n";
        break;
    default:
        result += "Unknown Palette command:"+subCommands[0].command.str+"\n";
        break;
    }
}

void MesDecoder::Move()
{
    if(!subCommands[0].command.isNumber)
    {
        result += "Unknown Move command:"+subCommands[0].command.str+"\n";
        return;
    }
    switch(subCommands[0].command.number)
    {
    case 0:
        result += "Move.init();\n";
        break;
    case 1:
        result += "Move.setBirthPos(doorIndex="+subCommands[1].command.str
                +", moveInfoIndex="+subCommands[2].command.str
                +", frameMaybe="+subCommands[3].command.str+");\n";
        break;
    case 2:
        result += "Move.getMapData();\n";
        break;
    case 3:
        result += "Move.resetBackupMapBuffer();\n";
        break;
    case 4:
        result += "Move.setMoveInfos();\n";
        break;
    case 5:
        result += "Move.setMoveInfo(index="+subCommands[1].command.str
                +", combOffset="+subCommands[2].command.str+");\n";
        break;
    case 6:
        result += "Move.setMyBlocks();\n";
        break;
    case 7:
        result += "Move.selectMoveInfo(index="+subCommands[1].command.str
                +", flag="+subCommands[2].command.str+");\n";
        break;
    case 8:
        result += "Move.move();\n";
        break;
    case 9:
        result += "reg16big.moveResult = Move.move();\n";
        break;
    case 10:
        result += "Move.paintInitMap();\n";
        break;
    case 11:
        result += "Move.painting();\n";
        break;
    case 12:
        result += "Move.unknown12("+subCommands[1].command.str+");\n";
        break;
    case 13:
        result += "reg16big.moveResult = Move.sub_4068F0();\n";
        break;
    case 14:
        result += "reg16big.moveResult = Move.sub_406A20("+subCommands[1].command.str+","+subCommands[2].command.str+");\n";
        break;
    case 15:
        result += "Move.unknown15("+subCommands[3].command.str+", "+subCommands[2].command.str+");\n";
        break;
    case 16:
        result += "Move.unknown16();\n";
        break;
    case 17:
        result += "Move.setMoveResult();\n";
        break;
    case 20:
        result += "Move.goBack(index="+subCommands[1].command.str+
                ", steps="+subCommands[2].command.str+");\n";
        break;
    case 21:
        result += "Move.setMoveCommand(index="+subCommands[1].command.str+");\n";
        break;
    case 22:
        result += "Move.setFlag(index="+subCommands[1].command.str+", flag=";
        if(subCommands[2].command.isNumber)
            result += QString::number(subCommands[2].command.number<<2);
        else
            result += subCommands[2].command.str+"<<2";
        result += ");\n";
        break;
    default:
        result += "Unknown Move command:"+subCommands[0].command.str+"\n";
        break;
    }
}

void MesDecoder::Cache()
{
    if(!subCommands[0].command.isNumber)
    {
        result += "Unknown Cache command:"+subCommands[0].command.str+"\n";
        return;
    }
    switch(subCommands[0].command.number)
    {
    case 0:
        result += "Cache.resetCursor(offset="+subCommands[1].command.str+");\n";
        break;
    case 1:
        result += "Cache.pushBack();\n";
        break;
    case 2:
        result += "Cache.add0();//maybe\n";
        break;
    case 3:
        result += "reg16bit = Cache.fildActive();\n";
        break;
    case 4:
        result += "reg32big.cacheCursor = Cache.cursor(index="+
                subCommands[1].command.str+");\n";
        break;
    default:
        result += "Unknown Cache command:"+subCommands[0].command.str+"\n";
        break;
    }
}

void MesDecoder::PaletteMisc()
{
    PrepareSubCommands();
    if(!subCommands[0].command.isNumber)
    {
        result += "Unknown PaletteMisc command:"+subCommands[0].command.str+"\n";
        return;
    }
    switch(subCommands[0].command.number)
    {
    case 0x5E:
        result += "PaletteMisc.backup(index="+subCommands[1].command.str+", count="
                +subCommands[2].command.str+");\n";
        break;
    case 0x5F:
        result += "PaletteMisc.restorePalette();\n";
        break;
    case 0x60:
        result += "PaletteMisc.getStoredColor();\n";
        break;
    case 0x61:
        result += "reg16bit.moveResult = checkInput(7);//PaletteMisc\n";
        break;
    case 0x63:
        result += "reg16bit.moveResult = checkInput(0);//PaletteMisc\n";
        break;
    case 0x64:
        result += "PaletteMisc.waitInput();\n";
        break;
    case 0x65:
        result += "PaletteMisc.resetTimer();\n";
        break;
    case 0x66:
        result += "pEngine->paletteAnimationSteps=16;//PaletteMisc\n";
        break;
    case 0x67:
        result += "PaletteMisc.waitMapMove();//maybe\n";
        break;
    case 0x6C:
        result += "PaletteMisc.setMBRightPressedMark();\n";
        break;
    case 0x6E:
        result += "PaletteMisc.loadStoredPalette6E(start="+subCommands[2].command.str
                +", count="+subCommands[1].command.str+");\n";
        break;
    case 0x70:
        result += "PaletteMisc.loadStoredPalette70(start="+subCommands[2].command.str
                +", count="+subCommands[1].command.str+");\n";
        break;
    case 0x71:
        result += "PaletteMisc.loadStoredPalette71(start="+subCommands[2].command.str
                +", count="+subCommands[1].command.str+");\n";
        break;
    case 0x73:
        result += "PaletteMisc.blackPalette();\n";
        break;
    case 0x78:
        result += "PaletteMisc.paletteAnimation78();\n";
        break;
    case 0x7A:
        result += "PaletteMisc.setPalettes();\n";
        break;
    case 0x7B:
        result += "PaletteMisc.paletteAnimation7B();\n";
        break;
    default:
        result += "Unknown PaletteMisc command:"+subCommands[0].command.str+"\n";
        break;
    }
}
