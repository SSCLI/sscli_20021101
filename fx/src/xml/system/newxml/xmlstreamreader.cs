//------------------------------------------------------------------------------
// <copyright file="XmlStreamReader.cs" company="Microsoft">
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
    using System;
    using System.IO;
    using System.Text;

    internal class XmlStreamReader: TextReader
    {
        internal Stream _Stream = null;

        internal XmlStreamReader(Stream stream) {
            _Stream = stream;
        }

        internal int Read(byte[] data, int offset, int length) {
            return _Stream.Read(data, offset, length);
        }

        public override void Close() {
            _Stream.Close();
            base.Close();
        }

        internal bool CanCalcLength {
            get { return _Stream != null && _Stream.CanSeek; }
        }

        internal long Length {
            get { return _Stream.Length; }
        }

    }
}

