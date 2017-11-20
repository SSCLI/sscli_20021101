//------------------------------------------------------------------------------
// <copyright file="IxmlLineInfo.cs" company="Microsoft">
//     
//      Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//     
//      The use and distribution terms for this software are contained in the file
//      named license.txt, which can be found in the root of this distribution.
//      By using this software in any fashion, you are agreeing to be bound by the
//      terms of this license.
//     
//      You must not remove this notice, or any other, from this software.
//     
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml 
{
/// <include file='doc\IXmlLineInfo.uex' path='docs/doc[@for="IXmlLineInfo"]/*' />
public interface IXmlLineInfo {
    /// <include file='doc\IXmlLineInfo.uex' path='docs/doc[@for="IXmlLineInfo.HasLineInfo"]/*' />
    bool HasLineInfo();
    /// <include file='doc\IXmlLineInfo.uex' path='docs/doc[@for="IXmlLineInfo.LineNumber"]/*' />
    int LineNumber { get; }
    /// <include file='doc\IXmlLineInfo.uex' path='docs/doc[@for="IXmlLineInfo.LinePosition"]/*' />
    int LinePosition { get; }
}


internal class PositionInfo : IXmlLineInfo {
    public virtual bool HasLineInfo() { return false; }
    public virtual int LineNumber { get { return 0;} }
    public virtual int LinePosition { get { return 0;} }

    public static PositionInfo GetPositionInfo(Object o) {
        if (o is IXmlLineInfo) {
            return new ReaderPositionInfo(o as IXmlLineInfo);
        }
        else {
            return new PositionInfo();
        }
    }

}

internal class ReaderPositionInfo: PositionInfo {

    private IXmlLineInfo mlineInfo;

    public ReaderPositionInfo(IXmlLineInfo lineInfo) {

        mlineInfo = lineInfo;

    }

    public override bool HasLineInfo() { return mlineInfo.HasLineInfo(); }
    public override int LineNumber { get { return mlineInfo.LineNumber;} }
    public override int LinePosition { get { return mlineInfo.LinePosition;} }

}
}// namespace
