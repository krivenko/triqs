################################################################################
#
# TRIQS: a Toolbox for Research in Interacting Quantum Systems
#
# Copyright (C) 2011 by M. Ferrero, O. Parcollet
#
# TRIQS is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# TRIQS is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# TRIQS. If not, see <http://www.gnu.org/licenses/>.
#
################################################################################

import lazy_expressions, descriptors
from block_gf import BlockGf
from gf import GfImFreq, GfReFreq, TailGf
from descriptor_base import A_Omega_Plus_B
import numpy as np
from itertools import product

def inverse(x):
    """
    Compute the inverse of a Green's function and return this.
    """
    if descriptors.is_lazy(x):
        return lazy_expressions.lazy_function("inverse", inverse) (x)
    assert hasattr(x,'invert') and hasattr(x,'copy')
    cop = x.copy() 
    cop.invert()
    return cop


def delta(g):
    """
    Compute Delta_iw from G0_iw.

    Parameters
    ----------
    g : BlockGf (of GfImFreq) or GfImFreq
        Non-interacting Green's function.

    Returns
    -------
    delta_iw : BlockGf (of GfImFreq) or GfImFreq
               Hybridization function.
    """
    if type(g) == BlockGf:
    	return BlockGf(name_block_generator = [ (n, delta(g0)) for n,g0 in g], make_copies=False)
    elif type(g) == GfImFreq:
    	g0_iw_inv = inverse(g)
    	delta_iw = g0_iw_inv.copy()
    	delta_iw << A_Omega_Plus_B(g0_iw_inv.tail[-1], g0_iw_inv.tail[0])
    	delta_iw -= g0_iw_inv
        return delta_iw
    else:
        raise TypeError, "No function delta for g0 object of type %s"%type(g) 


# Determine one of G0_iw, G_iw and Sigma_iw from other two using Dyson's equation
def dyson(**kwargs):
    """
    Solve Dyson's equation for given two of G0_iw, G_iw and Sigma_iw to yield the third.

    Parameters
    ----------
    G0_iw : Gf, optional
            Non-interacting Green's function.
    G_iw : Gf, optional
           Interacting Green's function.
    Sigma_iw : Gf, optional
               Self-energy.
    """
    if not (len(kwargs)==2 and set(kwargs.keys())<set(['G0_iw','G_iw', 'Sigma_iw'])):
        raise ValueError, 'dyson: Two (and only two) of G0_iw, G_iw and Sigma_iw must be provided to determine the third.'
    if 'G0_iw' not in kwargs:
        G0_iw = inverse(kwargs['Sigma_iw'] - inverse(kwargs['G_iw']))
        return G0_iw
    elif 'G_iw' not in kwargs:
        G_iw = inverse(inverse(kwargs['G0_iw']) - kwargs['Sigma_iw'])
        return G_iw
    elif 'Sigma_iw' not in kwargs:
        Sigma_iw = inverse(kwargs['G0_iw']) - inverse(kwargs['G_iw'])
        return Sigma_iw

# Fit tails for Sigma_iw and G_iw.
# Either give window to fix in terms of matsubara frequencies index (fit_min_n/fit_max_n) or value (fit_min_w/fit_max_w).
def tail_fit(Sigma_iw,G0_iw=None,G_iw=None,fit_min_n=None,fit_max_n=None,fit_min_w=None,fit_max_w=None,fit_max_moment=None,fit_known_moments=None):
    """
    Fit the tails of Sigma_iw and optionally G_iw.

    Parameters
    ----------
    Sigma_iw : Gf
               Self-energy.
    G0_iw : Gf, optional
            Non-interacting Green's function.
    G_iw : Gf, optional
           Interacting Green's function.
           If G0_iw and G_iw are provided, the tails of G_iw will also be fitted.
    fit_min_n : int, optional, default=int(0.8*len(Sigma_iw.mesh))
                Matsubara frequency index from which tail fitting should start.
    fit_max_n : int, optional, default=int(len(Sigma_iw.mesh))
                Matsubara frequency index at which tail fitting should end.
    fit_min_w : float, optional
                Matsubara frequency from which tail fitting should start.
    fit_max_w : float, optional
                Matsubara frequency at which tail fitting should end.
    fit_max_moment : int, optional
                     Highest moment to fit in the tail of Sigma_iw.
    fit_known_moments : dict{str:TailGf object}, optional, default = {block_name: TailGf(dim1, dim2, max_moment=0, order_min=0)}
                        Known moments of Sigma_iw, given as a TailGf object.

    Returns
    -------
    Sigma_iw : Gf
               Self-energy.
    G_iw : Gf, optional
           Interacting Green's function.
    """

    # Define default tail quantities
    if fit_known_moments is None:
        fit_known_moments = {name:TailGf(sig.N1,sig.N2,0,0) for name, sig in Sigma_iw}
    if fit_min_w is not None: fit_min_n = int(0.5*(fit_min_w*Sigma_iw.mesh.beta/np.pi - 1.0))
    if fit_max_w is not None: fit_max_n = int(0.5*(fit_max_w*Sigma_iw.mesh.beta/np.pi - 1.0))
    if fit_min_n is None: fit_min_n = int(0.8*len(Sigma_iw.mesh))
    if fit_max_n is None: fit_max_n = int(len(Sigma_iw.mesh))
    if fit_max_moment is None: fit_max_moment = 3

    # Now fit the tails of Sigma_iw and G_iw
    for name, sig in Sigma_iw: sig.fit_tail(fit_known_moments[name], fit_max_moment, fit_min_n, fit_max_n)
    if (G_iw is not None) and (G0_iw is not None):
        for name, g in G_iw: g.tail = inverse( inverse(G0_iw[name].tail) - Sigma_iw[name].tail )

    return Sigma_iw, G_iw


def read_gf_from_txt(block_txtfiles, block_name):
    """
    Read a GfReFreq from text files with the format (w, Re(G), Im(G)) for a single block.
    
    Notes
    -----
    A BlockGf must be constructed from multiple GfReFreq objects if desired.
    The mesh must be the same for all files read in.
    Non-uniform meshes are not supported.

    Parameters
    ----------
    block_txtfiles: Rank 2 square np.array(str) or list[list[str]]
        The text files containing the GF data that need to read for the block.
        e.g. [['up_eg1.dat']] for a one-dimensional block and
             [['up_eg1_1.dat','up_eg2_1.dat'],
              ['up_eg1_2.dat','up_eg2_2.dat']] for a 2x2 block.
    block_name: str
        Name of the block.

    Returns
    -------
    g: GfReFreq
        The real frequency Green's function read in.
    """
    block_txtfiles = np.array(block_txtfiles) # Must be an array to use certain functions
    N1,N2 = block_txtfiles.shape
    mesh = np.genfromtxt(block_txtfiles[0,0],usecols=[0]) # Mesh needs to be the same for all blocks
    g = GfReFreq(indices=range(N1),window=(np.min(mesh),np.max(mesh)),n_points=len(mesh),name=block_name)
    for i,j in product(range(N1),range(N2)):
        data = np.genfromtxt(block_txtfiles[i,j],usecols=[1,2])
        g.data[:,i,j] = data[:,0] + 1j*data[:,1]
    return g


def write_gf_to_txt(g):
    """
    Write a GfReFreq or GfImFreq to in the format (w/iw, Re(G), Im(G)) for a single block.
    
    Parameters
    ----------
    g: GfReFreq or GfImFreq
        The real/imaginary frequency Green's function to be written out.
    """
    if type(g) == GfReFreq:
        mesh = np.array(list(g.mesh)).real.reshape(-1,1)
    elif type(g) == GfImFreq:
        mesh = np.array(list(g.mesh)).imag.reshape(-1,1)
    else:
        raise ValueError, 'write_gf_to_txt: Only GfReFreq and GfImFreq quantities are supported.'
    for i,j in product(range(g.N1),range(g.N2)):
        txtfile = '%s_%s_%s.dat'%(g.name,i,j)
        redata = g.data[:,i,i].real.reshape((g.data.shape[0],-1))
        imdata = g.data[:,i,i].imag.reshape((g.data.shape[0],-1))
        mesh_and_data = np.hstack((mesh,redata,imdata))
        np.savetxt(txtfile,mesh_and_data)
