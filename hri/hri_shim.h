/* Minimal HRI shim for gossamer-based builds.
 * Provides only the hri_* macros used by stock_stopwatch_face and toss_up_face,
 * implemented via direct CMSIS register access using the gossamer chip headers. */
#ifndef _HRI_SHIM_H_
#define _HRI_SHIM_H_

/* MCLK */
#define hri_mclk_set_APBCMASK_TC2_bit(hw)   ((Mclk *)(hw))->APBCMASK.reg |= MCLK_APBCMASK_TC2
#define hri_mclk_set_APBCMASK_TRNG_bit(hw)  ((Mclk *)(hw))->APBCMASK.reg |= MCLK_APBCMASK_TRNG
#define hri_mclk_clear_APBCMASK_TRNG_bit(hw) ((Mclk *)(hw))->APBCMASK.reg &= ~MCLK_APBCMASK_TRNG

/* GCLK */
#define hri_gclk_write_PCHCTRL_reg(hw, index, val) ((Gclk *)(hw))->PCHCTRL[index].reg = (val)

/* TC (8-bit mode) */
#define hri_tc_set_CTRLA_ENABLE_bit(hw)     ((Tc *)(hw))->COUNT8.CTRLA.reg |= TC_CTRLA_ENABLE
#define hri_tc_clear_CTRLA_ENABLE_bit(hw)   ((Tc *)(hw))->COUNT8.CTRLA.reg &= ~TC_CTRLA_ENABLE
#define hri_tc_write_CTRLA_reg(hw, val)     ((Tc *)(hw))->COUNT8.CTRLA.reg = (val)
#define hri_tc_wait_for_sync(hw, mask)      while (((Tc *)(hw))->COUNT8.SYNCBUSY.reg & (mask))
#define hri_tc_set_INTEN_OVF_bit(hw)        ((Tc *)(hw))->COUNT8.INTENSET.reg |= TC_INTENSET_OVF
#define hri_tccount8_write_PER_reg(hw, val) ((Tc *)(hw))->COUNT8.PER.reg = (val)

/* TRNG */
#define hri_trng_set_CTRLA_ENABLE_bit(hw)          ((Trng *)(hw))->CTRLA.reg |= TRNG_CTRLA_ENABLE
#define hri_trng_get_INTFLAG_reg(hw, mask)          (((Trng *)(hw))->INTFLAG.reg & (mask))
#define hri_trng_read_DATA_reg(hw)                  ((Trng *)(hw))->DATA.reg

#endif /* _HRI_SHIM_H_ */
