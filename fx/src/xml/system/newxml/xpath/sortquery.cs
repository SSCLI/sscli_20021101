//------------------------------------------------------------------------------
// <copyright file="SortQuery.cs" company="Microsoft">
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

namespace System.Xml.XPath {
    using System;
    using System.Xml.Xsl;
    using System.Globalization;
    using System.Diagnostics;
    using System.Collections;
    using FT = System.Xml.XPath.Function.FunctionType;

    
    internal sealed class SortQuery : IQuery {
        private SortedList _Results;
        private ArrayList _sortExpressions;
        private ArrayList _comparers;
        private int _ResultCount = 0;
        private int _advance = 0;
        private IQuery _qyInput;
//        private XsltContext _context;


        internal SortQuery(IQuery  qyParent) {
            System.Diagnostics.Debug.Assert(qyParent != null, "Sort Query needs an input query tree to work on");
            _qyInput = qyParent;
            _sortExpressions = new ArrayList();
            _comparers = new ArrayList();
            _Results = new SortedList(new XPathComparer(_comparers));
        }
        
        internal SortQuery(SortQuery originalQuery) {
            this._qyInput = originalQuery._qyInput.Clone();
            int count = originalQuery._sortExpressions.Count;
            this._sortExpressions = new ArrayList(count);
            for (int i=0; i < count; i++) {
                IQuery query = originalQuery._sortExpressions[i] as IQuery;
                query = query.Clone();
                this._sortExpressions.Add(query);
            }
            this._comparers = new ArrayList(originalQuery._comparers);
            this._Results = new SortedList(new XPathComparer(_comparers));
            //            SetXsltContext(_context);


        }

        internal override XPathResultType ReturnType() {
            return XPathResultType.NodeSet;
        }

        internal override void setContext(XPathNavigator e) {
            reset();
            _qyInput.setContext(e);
        }


        internal override XPathNavigator peekElement() {
            System.Diagnostics.Debug.Assert(false, "Didn't expect this method to be called.");
            return null;
        }

        internal override Querytype getName() {
            return Querytype.Sort;
        }

        internal override void reset() {
            _Results.Clear();
            _ResultCount = 0;
            _advance = 0;
            _qyInput.reset();
        }

        internal override IQuery Clone() {
//            sortquery.SetXsltContext(_context);
            return new SortQuery(this);
        }

        internal override void SetXsltContext(XsltContext context) {
//            _context = context;
            //_qyInput.SetXsltContext(context);
        }

        internal override XPathNavigator advance() {
            if (_ResultCount == 0) {
                // build up a sorted list of results
                BuildResultsList();

                if (_ResultCount == 0) // if result count is still zero
                    return null;
            }
            if (_advance < _ResultCount) {
                return(XPathNavigator)(_Results.GetByIndex(_advance++));
            }
            else return null;
        }

        internal override int Position {
            get {
		        System.Diagnostics.Debug.Assert( _advance > 0, " Called Position before advance ");
                return ( _advance - 1 );
            }
        }
        
        private void BuildResultsList() {
            XPathNavigator eNext;
            SortKey key;
            Int32 numSorts = _sortExpressions.Count;

            System.Diagnostics.Debug.Assert(numSorts > 0, "Why was the sort query created?");

            while (true) {
                eNext = _qyInput.advance();

                if (eNext == null)
                    break;

                // if this is the first time i.e., the cache is empty
                // and if we an xslt context to work with
               /* if (_ResultCount == 0 && _context != null)
                    for (Int32 i=0; i<numSorts; i++)
                        ((IQuery)_sortExpressions[i]).SetXsltContext(_context);*/

                // create the special object that represent the composite key
                // This key object will act as a container for the primary key value,
                // secondary key value etc.
                key = new SortKey(numSorts);

                for (Int32 j=0; j<numSorts; j++) {
                    object keyval = ((IQuery)_sortExpressions[j]).getValue(_qyInput);
                    key.SetKeyValue(keyval, j);
                }

                _Results.Add(key, eNext.Clone());
                _ResultCount++;
            }
        }

        internal void AddSort(IQuery evalQuery, IComparer comparer) {
            if (evalQuery.ReturnType() == XPathResultType.NodeSet) {
                ArrayList argList = new ArrayList();
                argList.Add(evalQuery);
                _sortExpressions.Add(new StringFunctions(argList, FT.FuncString));
            }
            else
                _sortExpressions.Add(evalQuery);

            _comparers.Add(comparer);
        }

    } // class SortQuery


    internal sealed class SortKey {
        private Int32 _numKeyParts;
        private object[] _keyParts;

        internal SortKey(Int32 n) {
            _numKeyParts = n;
            _keyParts = (object[])Array.CreateInstance(typeof(object), n);
        }

        internal void SetKeyValue(object x, Int32 index) {
            _keyParts[index] = x;
        }

        internal object this[int index] { get { return _keyParts[index];}}

        internal Int32 NumKeyParts {
            get { return _numKeyParts; }
        }

#if DEBUG
        internal void Dump() {
            for (Int32 j=0; j<_numKeyParts; j++)
                System.Diagnostics.Debug.Write("..." + _keyParts[j]);
            System.Diagnostics.Debug.WriteLine("");
        }
 #endif

    } // class SortKey


    internal sealed class XPathComparerHelper : IComparer {
        private XmlSortOrder _order;
        private XmlCaseOrder _caseOrder;
        private CultureInfo _cinfo;
        private XmlDataType _dataType;

        internal XPathComparerHelper(
            XmlSortOrder order,
            XmlCaseOrder caseOrder,
            CultureInfo  cinfo,
            XmlDataType  dataType) {

            _order = order;
            _caseOrder = caseOrder;
            _cinfo = cinfo;
            _dataType = dataType;
        }
                
        public Int32 Compare(object x, object y) {
            Int32 sortOrder = (_order == XmlSortOrder.Ascending) ? 1 : -1;
            switch(_dataType) {
                case XmlDataType.Text:
#if DEBUG
                    System.Diagnostics.Debug.WriteLine("Performing string comparison...");
#endif
                    String s1 = Convert.ToString(x);
                    String s2 = Convert.ToString(y);
                    Int32 result = String.Compare(
                                           s1,
                                           s2,
                                           (_caseOrder == XmlCaseOrder.None) ? false : true,
                                           _cinfo);
                    if (result != 0 || _caseOrder == XmlCaseOrder.None)
                        return (sortOrder * result);

                    // If we came this far, it means that strings s1 and s2 are
                    // equal to each other when case is ignored. Now it's time to check
                    // and see if they differ in case only and take into account the user
                    // requested case order for sorting purposes.
                    Int32 caseOrder = (_caseOrder == XmlCaseOrder.LowerFirst) ? 1 : -1;
                    result = String.Compare(
                                           s1,
                                           s2,
                                           false,
                                           _cinfo);
                    return (caseOrder * result);

                case XmlDataType.Number:
#if DEBUG
                    System.Diagnostics.Debug.WriteLine("Performing number comparison...");
#endif
                    double r1 = XmlConvert.ToXPathDouble(x);
                    double r2 = XmlConvert.ToXPathDouble(y);

                    // trying to return the result of (r1 - r2) casted down
                    // to an Int32 can be dangerous. E.g 100.01 - 100.00 would result
                    // erroneously in zero when casted down to Int32.
                    if (r1 > r2) {
                        return (1*sortOrder);
                    }
                    else if (r1 < r2) {
                        return (-1*sortOrder);
                    }
                    else {
                        if (r1 == r2) {
                            return 0;
                        }
                        if (Double.IsNaN(r1)) {
                            if (Double.IsNaN(r2)) {
                                return 0;
                            }
                            //r2 is not NaN .NaN comes before any other number
                            return (-1*sortOrder);
                        }
                        //r2 is NaN. So it should come after r1
                        return (1*sortOrder);
                    }
                default:
                    throw new ArgumentException("x");
            } // switch
            
        } // Compare ()

    } // class XPathComparerHelper

    internal sealed class XPathComparer : IComparer {
        private ArrayList _comparers;

        internal XPathComparer(ArrayList comparers) {
            _comparers = comparers;
        }

        Int32 IComparer.Compare(object x, object y) {
            System.Diagnostics.Debug.Assert(x != null && y != null, "Oops!! what happened?");
            return Compare((SortKey)x, (SortKey)y);
        }

        public Int32 Compare(SortKey x, SortKey y) {
#if DEBUG
            System.Diagnostics.Debug.WriteLine("Comparing >>>>");
            x.Dump();
            y.Dump();
#endif
            int result = 0;
            for (Int32 i=0; i<x.NumKeyParts; i++) {
                result = ((IComparer)_comparers[i]).Compare(x[i], y[i]);
                if (result != 0)
                    break;
            }

#if DEBUG
            System.Diagnostics.Debug.WriteLine("result = " + result.ToString());
            System.Diagnostics.Debug.WriteLine("");
#endif
            // if after all comparisions, the two sort keys are still equal,
            // preserve the doc order
            return (result == 0) ? -1 : result;
        }

    } // class XPathComparer

} // namespace
