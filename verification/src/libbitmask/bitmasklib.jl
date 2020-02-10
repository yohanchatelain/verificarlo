module Bitmask

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

const McaBitmaskException = McaException{Bitmask}

struct Seed
    fixed::Bool
    value::Int
end

const tinymt64_library = "libtinymt64"

function _set_mca_mode(mode::McaMode)
    if !mode
        throw(McaBitmaskException.BadMode(mode, "bad mode"))
    else
        mcalib_operation_type = mode
    end
end

function _set_mca_precision(precison::Precision)
    if !(precision in McaPrecision)
        throw(McaBitmaskException.BadPrecsion(precision, "bad precision"))
    else
        mcalib_t = precision
    end
end

function _mca_rand()
    return ccall((:tinymt64_generate_doubleOO, tinymt64_library), Float64,
                 (Ptr{Tinymt64}), random_state)
end

function get_random_mask(x::AbstractFloat)
    mask = ccall((:tinymt64_generate_doubleOO, tinymt64_library), Float64,
                 (Ptr{Tinymt64}), random_state)

    if isa(x,  Float32)
        Union{NTuple{2, UInt32}, Float64}(mask)
        
end

function 
