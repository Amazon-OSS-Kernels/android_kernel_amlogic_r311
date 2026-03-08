include $(TOPDIR)/config.mk

OPENSSL_LIB = $(obj)libopenssl.o

SOBJS	=

CFLAGS += -I$(UFBL_PATH)/include -DSUPPORT_UBOOT \
	-I$(UFBL_PATH)/features/common_openssl \
	-I$(UFBL_PATH)/features/common_openssl/include \
	-I$(UFBL_PATH)/features/common_openssl/include/openssl \
	-I$(UFBL_PATH)/features/common_openssl/crypto \
	-I$(UFBL_PATH)/features/common_openssl/crypto/bn \
	-I$(UFBL_PATH)/features/common_openssl/crypto/asn1 \
	-I$(UFBL_PATH)/features/common_openssl/crypto/bf \
	-I$(UFBL_PATH)/features/common_openssl/crypto/conf \
	-I$(UFBL_PATH)/features/common_openssl/crypto/evp \
	-I$(UFBL_PATH)/features/common_openssl/crypto/modes \
	-I$(UFBL_PATH)/features/common_openssl/crypto/objects \
	-I$(UFBL_PATH)/features/common_openssl/crypto/ripemd \
	-I$(UFBL_PATH)/features/common_openssl/crypto/rsa \
	-I$(UFBL_PATH)/features/common_openssl/crypto/sha \
	-I$(UFBL_PATH)/features/common_openssl/crypto/x509v3

# Default OpenSSL CFLAGS
CFLAGS += -DOPENSSL_BN_ASM_MONT \
		-DAES_ASM \
		-DSHA1_ASM \
		-DSHA256_ASM \
		-DSHA512_ASM \
		-DNO_WINDOWS_BRAINDEATH \
		-DOPENSSL_IMPLEMENTS_strncasecmp \
		-DGETPID_IS_MEANINGLESS \
		-DOPENSSL_THREADS \
		-D_REENTRANT \
		-DDSO_DLFCN \
		-DHAVE_DLFCN_H \
		-DZLIB \
		-DL_ENDIAN \
		-DNO_SYS_TYPES_H

# Disable un-necessary crypto algos
CFLAGS += -DOPENSSL_NO_CAMELLIA \
		-DOPENSSL_NO_CAPIENG \
		-DOPENSSL_NO_CAST \
		-DOPENSSL_NO_CMS \
		-DOPENSSL_NO_STDIO \
		-DOPENSSL_NO_GMP \
		-DOPENSSL_NO_IDEA \
		-DOPENSSL_NO_JPAKE \
		-DOPENSSL_NO_MD2 \
		-DOPENSSL_NO_MDC2 \
		-DOPENSSL_NO_RC2 \
		-DOPENSSL_NO_RC4 \
		-DOPENSSL_NO_RC5 \
		-DOPENSSL_NO_BF \
		-DOPENSSL_NO_SHA0 \
		-DOPENSSL_NO_RFC3779 \
		-DOPENSSL_NO_SEED \
		-DOPENSSL_NO_STORE \
		-DOPENSSL_NO_WHIRLPOOL \
		-DOPENSSL_NO_MD4 \
		-DOPENSSL_NO_MD5 \
		-DOPENSSL_NO_DES \
		-DOPENSSL_NO_OCSP \
		-DOPENSSL_NO_AES \
		-DOPENSSL_NO_DSA \
		-DOPENSSL_NO_DH \
		-DOPENSSL_NO_SHA512 \
		-DOPENSSL_NO_HW \
		-DOPENSSL_NO_ENGINE \
		-DOPENSSL_NO_EC2M \
		-DOPENSSL_NO_PKCS12 \
		-DOPENSSL_NO_EC \
		-DOPENSSL_NO_ECDSA \

CFLAGS += -DOPENSSL_SYS_LK -DOPENSSL_NO_HMAC -DNO_STDERR -DNO_ABORT -DNO_TIME \
		-DNO_STRCASECMP -DOPENSSL_NO_FP_API -DOPENSSL_NO_SSCANF -DNO_SSLV23 \
		-DOPENSSL_NO_OBJECT

ifeq (yes,$(strip $(UFBL_PROJ_ABC)))
CFLAGS += -DUFBL_PROJ_ABC
endif

# Do not link un-used functions
CFLAGS += -ffunction-sections -fdata-sections

COBJS-y += \
	crypto/cryptlib.o \
	crypto/mem.o \
	crypto/mem_clr.o \
	crypto/mem_dbg.o \
	crypto/cversion.o \
	crypto/ex_data.o \
	crypto/cpt_err.o \
	crypto/ebcdic.o \
	crypto/o_str.o \
	crypto/o_init.o \
	crypto/asn1/a_bitstr.o \
	crypto/asn1/a_bool.o \
	crypto/asn1/a_bytes.o \
	crypto/asn1/a_d2i_fp.o \
	crypto/asn1/a_digest.o \
	crypto/asn1/a_dup.o \
	crypto/asn1/a_enum.o \
	crypto/asn1/a_i2d_fp.o \
	crypto/asn1/a_int.o \
	crypto/asn1/a_mbstr.o \
	crypto/asn1/a_object.o \
	crypto/asn1/a_octet.o \
	crypto/asn1/a_print.o \
	crypto/asn1/a_sign.o \
	crypto/asn1/a_strnid.o \
	crypto/asn1/a_strex.o \
	crypto/asn1/a_time.o \
	crypto/asn1/a_type.o \
	crypto/asn1/a_utctm.o \
	crypto/asn1/a_utf8.o \
	crypto/asn1/a_verify.o \
	crypto/asn1/ameth_lib.o \
	crypto/asn1/asn1_err.o \
	crypto/asn1/asn1_lib.o \
	crypto/asn1/asn1_par.o \
	crypto/asn1/asn_mime.o \
	crypto/asn1/asn_moid.o \
	crypto/asn1/asn_pack.o \
	crypto/asn1/bio_asn1.o \
	crypto/asn1/d2i_pr.o \
	crypto/asn1/d2i_pu.o \
	crypto/asn1/evp_asn1.o \
	crypto/asn1/f_enum.o \
	crypto/asn1/f_int.o \
	crypto/asn1/f_string.o \
	crypto/asn1/i2d_pr.o \
	crypto/asn1/i2d_pu.o \
	crypto/asn1/n_pkey.o \
	crypto/asn1/p5_pbe.o \
	crypto/asn1/p8_pkey.o \
	crypto/asn1/t_bitst.o \
	crypto/asn1/t_crl.o \
	crypto/asn1/t_req.o \
	crypto/asn1/t_spki.o \
	crypto/asn1/t_x509.o \
	crypto/asn1/t_x509a.o \
	crypto/asn1/tasn_enc.o \
	crypto/asn1/tasn_dec.o \
	crypto/asn1/tasn_fre.o \
	crypto/asn1/tasn_new.o \
	crypto/asn1/tasn_prn.o \
	crypto/asn1/tasn_typ.o \
	crypto/asn1/tasn_utl.o \
	crypto/asn1/x_algor.o \
	crypto/asn1/x_attrib.o \
	crypto/asn1/x_bignum.o \
	crypto/asn1/x_crl.o \
	crypto/asn1/x_exten.o \
	crypto/asn1/x_info.o \
	crypto/asn1/x_long.o \
	crypto/asn1/x_name.o \
	crypto/asn1/x_nx509.o \
	crypto/asn1/x_pkey.o \
	crypto/asn1/x_pubkey.o \
	crypto/asn1/x_req.o \
	crypto/asn1/x_sig.o \
	crypto/asn1/x_spki.o \
	crypto/asn1/x_val.o \
	crypto/asn1/x_x509.o \
	crypto/asn1/x_x509a.o \
	crypto/bio/bio_lib.o \
	crypto/bio/b_print.o \
	crypto/bn/bn_add.o \
	crypto/bn/bn_asm.o \
	crypto/bn/bn_blind.o \
	crypto/bn/bn_ctx.o \
	crypto/bn/bn_div.o \
	crypto/bn/bn_err.o \
	crypto/bn/bn_exp.o \
	crypto/bn/bn_exp2.o \
	crypto/bn/bn_gcd.o \
	crypto/bn/bn_gf2m.o \
	crypto/bn/bn_kron.o \
	crypto/bn/bn_lib.o \
	crypto/bn/bn_mod.o \
	crypto/bn/bn_mont.o \
	crypto/bn/bn_mpi.o \
	crypto/bn/bn_mul.o \
	crypto/bn/bn_nist.o \
	crypto/bn/bn_prime.o \
	crypto/bn/bn_print.o \
	crypto/bn/bn_recp.o \
	crypto/bn/bn_shift.o \
	crypto/bn/bn_sqr.o \
	crypto/bn/bn_sqrt.o \
	crypto/bn/bn_word.o \
	crypto/buffer/buf_err.o \
	crypto/buffer/buf_str.o \
	crypto/buffer/buffer.o \
	crypto/conf/conf_api.o \
	crypto/conf/conf_def.o \
	crypto/conf/conf_err.o \
	crypto/conf/conf_lib.o \
	crypto/conf/conf_mall.o \
	crypto/err/err.o \
	crypto/err/err_all.o \
	crypto/err/err_prn.o \
	crypto/evp/c_all.o \
	crypto/evp/c_allc.o \
	crypto/evp/c_alld.o \
	crypto/evp/digest.o \
	crypto/evp/e_aes.o \
	crypto/evp/e_bf.o \
	crypto/evp/e_des.o \
	crypto/evp/e_des3.o \
	crypto/evp/e_null.o \
	crypto/evp/e_old.o \
	crypto/evp/e_rc2.o \
	crypto/evp/e_rc4.o \
	crypto/evp/e_rc5.o \
	crypto/evp/e_xcbc_d.o \
	crypto/evp/encode.o \
	crypto/evp/evp_acnf.o \
	crypto/evp/evp_enc.o \
	crypto/evp/evp_err.o \
	crypto/evp/evp_key.o \
	crypto/evp/evp_lib.o \
	crypto/evp/evp_pkey.o \
	crypto/evp/m_dss.o \
	crypto/evp/m_dss1.o \
	crypto/evp/m_mdc2.o \
	crypto/evp/m_null.o \
	crypto/evp/m_ripemd.o \
	crypto/evp/m_sha1.o \
	crypto/evp/m_sigver.o \
	crypto/evp/m_wp.o \
	crypto/evp/names.o \
	crypto/evp/p5_crpt.o \
	crypto/evp/p5_crpt2.o \
	crypto/evp/p_dec.o \
	crypto/evp/p_enc.o \
	crypto/evp/p_lib.o \
	crypto/evp/p_open.o \
	crypto/evp/p_seal.o \
	crypto/evp/p_sign.o \
	crypto/evp/p_verify.o \
	crypto/evp/pmeth_fn.o \
	crypto/evp/pmeth_gn.o \
	crypto/evp/pmeth_lib.o \
	crypto/lhash/lh_stats.o \
	crypto/lhash/lhash.o \
	crypto/modes/cbc128.o \
	crypto/modes/cfb128.o \
	crypto/modes/ofb128.o \
	crypto/objects/obj_dat.o \
	crypto/objects/obj_err.o \
	crypto/objects/obj_lib.o \
	crypto/objects/obj_xref.o \
	crypto/objects/o_names.o \
	crypto/rsa/rsa_ameth.o \
	crypto/rsa/rsa_asn1.o \
	crypto/rsa/rsa_chk.o \
	crypto/rsa/rsa_crpt.o \
	crypto/rsa/rsa_eay.o \
	crypto/rsa/rsa_err.o \
	crypto/rsa/rsa_gen.o \
	crypto/rsa/rsa_lib.o \
	crypto/rsa/rsa_none.o \
	crypto/rsa/rsa_null.o \
	crypto/rsa/rsa_oaep.o \
	crypto/rsa/rsa_pk1.o \
	crypto/rsa/rsa_pmeth.o \
	crypto/rsa/rsa_prn.o \
	crypto/rsa/rsa_pss.o \
	crypto/rsa/rsa_sign.o \
	crypto/rsa/rsa_saos.o \
	crypto/rsa/rsa_x931.o \
	crypto/sha/sha1_one.o \
	crypto/sha/sha1dgst.o \
	crypto/sha/sha256.o \
	crypto/sha/sha512.o \
	crypto/sha/sha_dgst.o \
	crypto/sha/asm/sha1-armv4-large.o \
	crypto/sha/asm/sha256-armv4.o \
	crypto/stack/stack.o \
	crypto/ts/ts_err.o \
	crypto/txt_db/txt_db.o \
	crypto/x509/x509_att.o \
	crypto/x509/x509_cmp.o \
	crypto/x509/x509_d2.o \
	crypto/x509/x509_def.o \
	crypto/x509/x509_err.o \
	crypto/x509/x509_ext.o \
	crypto/x509/x509_lu.o \
	crypto/x509/x509_obj.o \
	crypto/x509/x509_r2x.o \
	crypto/x509/x509_req.o \
	crypto/x509/x509_set.o \
	crypto/x509/x509_trs.o \
	crypto/x509/x509_txt.o \
	crypto/x509/x509_v3.o \
	crypto/x509/x509_vfy.o \
	crypto/x509/x509_vpm.o \
	crypto/x509/x509cset.o \
	crypto/x509/x509name.o \
	crypto/x509/x509rset.o \
	crypto/x509/x509spki.o \
	crypto/x509/x509type.o \
	crypto/x509/x_all.o \
	crypto/x509v3/pcy_cache.o \
	crypto/x509v3/pcy_data.o \
	crypto/x509v3/pcy_lib.o \
	crypto/x509v3/pcy_map.o \
	crypto/x509v3/pcy_node.o \
	crypto/x509v3/pcy_tree.o \
	crypto/x509v3/v3_akey.o \
	crypto/x509v3/v3_akeya.o \
	crypto/x509v3/v3_alt.o \
	crypto/x509v3/v3_bcons.o \
	crypto/x509v3/v3_bitst.o \
	crypto/x509v3/v3_conf.o \
	crypto/x509v3/v3_crld.o \
	crypto/x509v3/v3_enum.o \
	crypto/x509v3/v3_genn.o \
	crypto/x509v3/v3_int.o \
	crypto/x509v3/v3_lib.o \
	crypto/x509v3/v3_ncons.o \
	crypto/x509v3/v3_pcons.o \
	crypto/x509v3/v3_pku.o \
	crypto/x509v3/v3_pmaps.o \
	crypto/x509v3/v3_prn.o \
	crypto/x509v3/v3_purp.o \
	crypto/x509v3/v3_skey.o \
	crypto/x509v3/v3err.o \
	crypto/x509v3/v3_utl.o \

SOBJS = \
	crypto/bn/asm/armv4-mont.o

COBJS	= $(COBJS-y)
SRCS 	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(SOBJS) $(COBJS))

ifneq ($(OBJTREE),$(SRCTREE))
# Creating subdirs in $(OBJTREE)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/asn1)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/bio)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/bn)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/bn/asm)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/buffer)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/conf)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/err)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/evp)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/lhash)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/modes)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/objects)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/rsa)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/sha)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/sha/asm)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/stack)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/ts)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/txt_db)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/x509)
$(shell mkdir $(OBJTREE)/ufbl-features/features/common_openssl/crypto/x509v3)
endif

PLATFORM_CPPFLAGS+=$(CFLAGS)

$(OPENSSL_LIB): $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
#include $(SRCTREE)/rules.mk

# sinclude $(obj).depend

#########################################################################
