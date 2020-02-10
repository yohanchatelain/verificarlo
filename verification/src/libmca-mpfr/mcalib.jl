module Mpfr

@enum Operation begin
    add
    sub
    mul
    div
end

@enum McaMode begin
    ieee
    mca
    pb
    rr
end

struct Tinymt64
    status::NTuple{2, Clong}
    mat1::Cint
    mat2::Cint
    tmat::Clong
end

random_state = Ptr{Tinymt64}()

const McaMpfrException = McaException{Mpfr}

struct Seed
    fixed::Bool
    value::Int
end

const tinymt64_library = "libtinymt64"

function _set_mca_mode(mode::McaMode)
    if !mode
        throw(McaMpfrException.BadMode(mode, "bad mode"))
    else
        mcalib_operation_type = mode
    end
end

function _set_mca_precision(precison::Precision)
    if !(precision in McaPrecision)
        throw(McaMpfrException.BadPrecsion(precision, "bad precision"))
    else
        mcalib_t = precision
    end
end

function _mca_rand()
    return ccall((:tinymt64_generate_doubleOO, tinymt64_library), Float64,
                 (Ptr{Tinymt64}), random_state)
end

function _mca_inexact(a::BigFloat)
    if mcamode == ieee || isNormalFloatNumber(a)
        return a
    end
    exponent_a = exponent(a) - (mcalib_t - 1)
    precision_a = precision(a)
    random_float64 = _mca_rand() - 0.5
    random = random * 2.0 ^ exponent_a
    return a + random
end

function _mca_seed(seed::Seed)
    
    if !seed.fixed
        init_key = rand(Uint, 3)
    else
        init_key = fill(seed.value, 3)
    end 

    ccall((:tinymt64_init_by_array, tinymt64_library), Cvoid,
          (Ptr{Tinymt64}, NTuple{3, Cint}, Cint),
          random_state, init_key, 3)
end

function _mca_bin(a::AbstractFloat, b::AbstractFloat, op::Operation)
    if mcamode != rr
        a = _mca_inexact(a)
        b = _mca_inexact(b)
    end
    r = op(a,b)
    if mcamode != pb
        r = _mca_inexact(r)
    end
    return r
end

function _mca_unr(a::AbstractFloat, op::Operation)
    if mcamode != rr
        a = _mca_inexact(a)
    end
    r = op(a)
    if mcamode != pb
        r = _mca_inexact(r)
    end
    return r
end



           
