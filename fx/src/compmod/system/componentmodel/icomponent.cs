//------------------------------------------------------------------------------
// <copyright file="IComponent.cs" company="Microsoft">
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

namespace System.ComponentModel {
    using System;

    // A "component" is an object that can be placed in a container.
    //
    // In this context, "containment" refers to logical containment, not visual
    // containment.  Components and containers can be used in a variety of
    // scenarios, including both visual and non-visual scenarios.
    //
    // To be a component, a class implements the IComponent interface, and provides
    // a parameter-less constructor.
    //
    // A component interacts with its container primarily through a container-
    // provided "site".

    // Interfaces don't need to be serializable
    /// <include file='doc\IComponent.uex' path='docs/doc[@for="IComponent"]/*' />
    /// <devdoc>
    ///    <para>Provides functionality required by all components.</para>
    /// </devdoc>
    public interface IComponent : IDisposable {
        // The site of the component.
        /// <include file='doc\IComponent.uex' path='docs/doc[@for="IComponent.Site"]/*' />
        /// <devdoc>
        ///    <para>When implemented by a class, gets or sets
        ///       the <see cref='System.ComponentModel.ISite'/> associated
        ///       with the <see cref='System.ComponentModel.IComponent'/>.</para>
        /// </devdoc>
        ISite Site {
            get;
            set;
        }

        /// <include file='doc\IComponent.uex' path='docs/doc[@for="IComponent.Disposed"]/*' />
        /// <devdoc>
        ///    <para>Adds a event handler to listen to the Disposed event on the component.</para>
        /// </devdoc>
        event EventHandler Disposed;
    }
}
